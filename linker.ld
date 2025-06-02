ENTRY(_start)


SECTIONS {
  . = 0x00100000; /* Physical load address */

  .multiboot : {
    KEEP(*(.multiboot))
    }

  /* Boot stub in low memory */
  .text.stub : {
    KEEP(*(.text.stub))
  }

  .data.stub : {
    *(.data.stub)
  }

  /* Stack for stub */
  
  
  .stack (NOLOAD) : {
    . = ALIGN(16);
    __stack_bottom = .;
    . = . + 0x1000; /* 4 KB stack */
     __stack_top = .;
  }
  __stack_top = .;

  /* High-half kernel: linked at 0xC0000000 */
  . = 0xC0000000;

 .text : AT(ADDR(.text) - 0xC0000000 + 0x00102000) {
  *(.text)
}

.rodata : AT(ADDR(.rodata) - 0xC0000000 + 0x00102000) {
  *(.rodata)
}

.data : AT(ADDR(.data) - 0xC0000000 + 0x00102000) {
  *(.data)
}

.bss : AT(ADDR(.bss) - 0xC0000000 + 0x00102000) {
  *(COMMON)
  *(.bss)
}

  _kernel_end = .;
  
    PROVIDE(__stack_top = __stack_top);
}