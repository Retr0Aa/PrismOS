.section .note.GNU-stack,"",@progbits

.code32

.section .multiboot2_header,"a"
.align 8
header_start:
.long 0xE85250D6
.long 0
.long header_end - header_start
.long -(0xE85250D6 + 0 + (header_end - header_start))

.align 8
.short 5
.short 0
.long 20
.long 800
.long 600
.long 32

.align 8
.short 0
.short 0
.long 8
header_end:

.section .data
.global g_multiboot_magic
.global g_multiboot_info
g_multiboot_magic:
    .long 0
g_multiboot_info:
    .long 0

.section .text
.global start
.extern main

start:
    movl %eax, g_multiboot_magic
    movl %ebx, g_multiboot_info

    mov $stack_top, %esp
    call main

hang:
    cli
    hlt
    jmp hang


.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
