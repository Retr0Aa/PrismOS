#include "paging.h"

#include <stdint.h>

#include "debug/log.h"
#include "display/console.h"
#include "interrupts/irq.h"
#include "platform/io.h"

#define PAGE_SIZE 4096U
#define PAGE_ENTRIES 1024U
#define BOOTSTRAP_PT_COUNT 128U
#define IDENTITY_BOOT_END 0x04000000U

#define PAGE_DIR_INDEX(x) (((x) >> 22) & 0x3FFU)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3FFU)
#define PAGE_ALIGN_DOWN(x) ((x) & 0xFFFFF000U)

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

static uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
/* Early boot uses a static page-table pool to avoid allocator recursion while
 * PMM and paging are brought up together. */
static uint32_t page_table_pool[BOOTSTRAP_PT_COUNT][PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_table_pool_used = 0U;

static int paging_enabled = 0;
static int paging_fault_handler_registered = 0;

static void memory_zero(void *ptr, uint32_t size)
{
    uint8_t *out = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; ++i) {
        out[i] = 0;
    }
}

static void page_fault_handler(registers_t *regs)
{
    /* Keep this simple for now; richer diagnostics and recovery can be added
     * once fault handling policies are introduced. */
    (void)regs;
    (void)read_cr2();
    ERROR_LOG("page_fault_handler: page fault detected");
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

static uint32_t *ensure_page_table(uint32_t virt)
{
    uint32_t dir_index = PAGE_DIR_INDEX(virt);

    if ((page_directory[dir_index] & PAGING_FLAG_PRESENT) == 0U) {
        if (page_table_pool_used >= BOOTSTRAP_PT_COUNT) {
            ERROR_LOG("paging: out of memory allocating page table");
            return 0;
        }

        uint32_t *table_virt = page_table_pool[page_table_pool_used++];
        uint32_t table_phys = (uint32_t)(uintptr_t)table_virt;
        memory_zero(table_virt, PAGE_SIZE);

        page_directory[dir_index] = table_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_READ_WRITE;
    }

    return (uint32_t *)(uintptr_t)(page_directory[dir_index] & 0xFFFFF000U);
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t *table = ensure_page_table(virt);
    if (table == 0) {
        return;
    }

    uint32_t table_index = PAGE_TABLE_INDEX(virt);
    table[table_index] = PAGE_ALIGN_DOWN(phys) | (flags & 0xFFFU) | PAGING_FLAG_PRESENT;

    if (paging_enabled) {
        invlpg((void *)(uintptr_t)PAGE_ALIGN_DOWN(virt));
    }
}

void paging_unmap(uint32_t virt)
{
    uint32_t dir_index = PAGE_DIR_INDEX(virt);
    if ((page_directory[dir_index] & PAGING_FLAG_PRESENT) == 0U) {
        return;
    }

    uint32_t *table = (uint32_t *)(uintptr_t)(page_directory[dir_index] & 0xFFFFF000U);
    table[PAGE_TABLE_INDEX(virt)] = 0U;

    if (paging_enabled) {
        invlpg((void *)(uintptr_t)PAGE_ALIGN_DOWN(virt));
    }
}

uint32_t paging_get_physical(uint32_t virt)
{
    uint32_t dir_index = PAGE_DIR_INDEX(virt);
    if ((page_directory[dir_index] & PAGING_FLAG_PRESENT) == 0U) {
        return 0U;
    }

    uint32_t *table = (uint32_t *)(uintptr_t)(page_directory[dir_index] & 0xFFFFF000U);
    uint32_t entry = table[PAGE_TABLE_INDEX(virt)];

    if ((entry & PAGING_FLAG_PRESENT) == 0U) {
        return 0U;
    }

    return (entry & 0xFFFFF000U) | (virt & 0xFFFU);
}

static void map_identity_range(uint32_t start, uint32_t end, uint32_t flags)
{
    uint32_t current = PAGE_ALIGN_DOWN(start);
    uint32_t final = (end + PAGE_SIZE - 1U) & 0xFFFFF000U;

    while (current < final) {
        paging_map(current, current, flags);
        current += PAGE_SIZE;
    }
}

void paging_init(const FramebufferInfo *framebuffer)
{
    memory_zero(page_directory, sizeof(page_directory));
    page_table_pool_used = 0U;

    /* Identity-map a conservative early region so existing physical-pointer
     * code continues to run after CR0.PG is enabled. */
    map_identity_range(0x00000000U, IDENTITY_BOOT_END, PAGING_FLAG_READ_WRITE);

    uint32_t kernel_start = (uint32_t)(uintptr_t)&__kernel_start;
    uint32_t kernel_end = (uint32_t)(uintptr_t)&__kernel_end;
    map_identity_range(kernel_start, kernel_end, PAGING_FLAG_READ_WRITE);

    if (framebuffer != 0) {
        uint64_t fb_start64 = framebuffer->address;
        uint64_t fb_size64 = (uint64_t)framebuffer->pitch * (uint64_t)framebuffer->height;
        uint64_t fb_end64 = fb_start64 + fb_size64;

        if (fb_start64 <= 0xFFFFFFFFULL) {
            if (fb_end64 > 0x100000000ULL) {
                fb_end64 = 0x100000000ULL;
            }
            map_identity_range((uint32_t)fb_start64, (uint32_t)fb_end64, PAGING_FLAG_READ_WRITE);
        }
    }

    write_cr3((uint32_t)(uintptr_t)page_directory);
    write_cr0(read_cr0() | 0x80000000U);

    paging_enabled = 1;

    if (!paging_fault_handler_registered) {
        exception_register_handler(14U, page_fault_handler);
        paging_fault_handler_registered = 1;
    }

    DEBUG_LOG("paging_init: paging enabled");
}
