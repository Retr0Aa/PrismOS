ENTRY(start)

SECTIONS
{
    __kernel_start = .;

    .multiboot : {
        KEEP(*(.multiboot))
    }

    . = 1M;

    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON) }

    __kernel_end = .;
}
