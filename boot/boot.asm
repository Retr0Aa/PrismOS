[org 0x7C00]
[bits 16]

mov [BOOT_DRIVE], dl

; Detect total installed RAM and store it where the kernel can read it.
; Value is stored in kilobytes at physical address 0x9000.
mov dword [0x9000], 0

mov ax, 0xE801
int 0x15
jc .fallback_memory_query

xor eax, eax
movzx eax, ax
movzx ecx, bx
shl ecx, 6
add eax, ecx
add eax, 1024
mov [0x9000], eax
jmp .memory_query_done

.fallback_memory_query:
mov ax, 0x8800
int 0x15
jc .memory_query_done

xor eax, eax
movzx eax, ax
add eax, 1024
mov [0x9000], eax

.memory_query_done:

; Load kernel into memory at 0x1000
xor ax, ax
mov es, ax
mov bx, 0x1000

mov ah, 0x02    ; BIOS read sectors
mov al, 20      ; Read 20 sectors
mov ch, 0       ; Cylinder 0
mov cl, 2       ; Start at sector 2
mov dh, 0       ; Head 0
mov dl, [BOOT_DRIVE]
int 0x13

; Disable interrupts
cli

; Load GDT
lgdt [gdt_descriptor]

; Enable protected mode
mov eax, cr0
or eax, 1
mov cr0, eax

; Far jump to 32-bit code
jmp CODE_SEG:0x1000

BOOT_DRIVE db 0

; GDT (Global Descriptor Table)
gdt_start:
    ; Null descriptor (required)
    dq 0x0

    ; Code segment (0x08)
    dw 0xFFFF           ; Limit (low)
    dw 0x0000           ; Base (low)
    db 0x00             ; Base (mid)
    db 0b10011010       ; Access byte (code)
    db 0b11001111       ; Flags and limit (high)
    db 0x00             ; Base (high)

    ; Data segment (0x10)
    dw 0xFFFF           ; Limit (low)
    dw 0x0000           ; Base (low)
    db 0x00             ; Base (mid)
    db 0b10010010       ; Access byte (data)
    db 0b11001111       ; Flags and limit (high)
    db 0x00             ; Base (high)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ 0x08

times 510-($-$$) db 0
dw 0xAA55