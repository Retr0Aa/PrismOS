#ifndef PRISMOS_PAGING_H
#define PRISMOS_PAGING_H

#include <stdint.h>

#include "display/console.h"

#define PAGING_FLAG_PRESENT       0x001U
#define PAGING_FLAG_READ_WRITE    0x002U
#define PAGING_FLAG_USER          0x004U
#define PAGING_FLAG_WRITE_THROUGH 0x008U
#define PAGING_FLAG_CACHE_DISABLE 0x010U
#define PAGING_FLAG_GLOBAL        0x100U

/* Enable early x86 paging. This implementation uses identity mapping in
 * the bootstrap phase. */
void paging_init(const FramebufferInfo *framebuffer);

/* Map/unmap virtual pages to physical pages using 4 KiB granularity.
 * Addresses should be page-aligned by callers for predictable behavior. */
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_unmap(uint32_t virt);

uint32_t paging_get_physical(uint32_t virt);

#endif /* PRISMOS_PAGING_H */
