ENTRY(_start)

SECTIONS
{
    . = 0x1000;

    /* Multiboot header must be in first 8KB */
    .multiboot : { *(.multiboot) }
    
    /* Read-only sections */
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    
    /* Read-write sections */
    . = ALIGN(0x1000);
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}