ENTRY(_start)

KERNEL_VIRT_BASE = 0xC0000000;
KERNEL_PHYS_BASE = 0x00100000;

SECTIONS
{
    . = KERNEL_VIRT_BASE;
    kernel_start = .;

    .multiboot2 ALIGN(0x1000) : AT(KERNEL_PHYS_BASE) {
        KEEP(*(.multiboot2))
    }

     .early_init ALIGN(0x1000) : AT(LOADADDR(.multiboot2) + SIZEOF(.multiboot2)) {
        *(.early_init)
    }


    .text ALIGN(0x1000) : {
        *(.text*)
    }

    .rodata ALIGN(0x1000) : {
        *(.rodata*)
    }

    .data ALIGN(0x1000) : {
        *(.data*)
    }

    .bss ALIGN(0x1000) : {
        *(.bss*)
        *(COMMON)
    }

    _kernel_end = .;
}