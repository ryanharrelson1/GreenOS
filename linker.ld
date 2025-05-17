ENTRY(_start);




SECTIONS {
     kernel_start = .;

    . = 1M;

    
    .multiboot2 :
    {
        KEEP(*(.multiboot2))
    }

    .text : {

        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    _kernel_end = .;
}