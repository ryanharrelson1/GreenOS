[BITS 32]
[GLOBAL _start]
[EXTERN kernel_main]
[GLOBAL stack_top]



SECTION .multiboot2
align 8
multiboot_header_start:
    dd 0xE85250D6       ;magic
    dd 0                ; (architecture)
    dd multiboot_header_end - multiboot_header_start ; header len
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start)) ; checksum

    ; --- Tag: Memory Map Request ---
    dw 6 
    dw 0
    dd 16
    dd 24
    dd 0

    ; end tags
    dw 0
    dw 0
    dd 8
multiboot_header_end:


SECTION .text

_start:

    ; set stack top
    mov esp, stack_top

    push ebx
    

    ; call c kernel_main

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

SECTION .bss
align 16
stack_bottom:
    resb 16384   ; 16 kb stack 
stack_top: