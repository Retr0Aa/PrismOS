[bits 32]

extern main

; GNU stack marking (suppresses "missing .note.GNU-stack" warning)
section .note.GNU-stack progbits
    
; Multiboot header (must be in first 8KB of ELF, 4-byte aligned)
section .multiboot align=4
    dd 0x1BADB002           ; Magic number
    dd 0x00000003           ; Flags: bit 0 (align modules), bit 1 (memory info)
    dd -(0x1BADB002 + 0x00000003)  ; Checksum (makes magic + flags + checksum = 0)

; Global variable to store multiboot_info pointer
section .data
    global g_multiboot_magic
    global g_multiboot_info
    g_multiboot_magic: dd 0
    g_multiboot_info: dd 0

section .text
    global _start

_start:
    ; Save multiboot info before using EAX/EBX
    mov [g_multiboot_magic], eax    ; EAX = magic from GRUB (0x2BADB002)
    mov [g_multiboot_info], ebx     ; EBX = pointer to multiboot_info struct
    
    ; Set up segment registers for 32-bit protected mode
    ; GRUB already provides a working GDT, but we still set segment regs
    mov ax, 0x10        ; Data segment selector (from GRUB's GDT)
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; Set up stack pointer
    mov esp, 0x90000    ; Stack at 0x90000 (below kernel)
    
    ; Call main (kernel.c)
    call main
    
    ; Infinite loop if main returns
    jmp $
