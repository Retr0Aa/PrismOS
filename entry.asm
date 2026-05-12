[bits 32]

extern main

section .text
    global _start

_start:
    ; Set up segment registers for 32-bit protected mode
    mov ax, 0x10        ; Data segment selector
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    ; Set up stack pointer
    mov esp, 0x90000    ; Stack at 0x90000 (below kernel)
    
    ; Call main
    call main
    
    ; Infinite loop if main returns
    jmp $
