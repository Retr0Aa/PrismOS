.section .note.GNU-stack,"",@progbits

.code32

.section .multiboot,"a"
.align 4
.long 0x1BADB002
.long 0x00000003
.long -(0x1BADB002 + 0x00000003)

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
