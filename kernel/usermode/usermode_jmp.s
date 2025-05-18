[BITS 32]

global enter_user_mode

enter_user_mode:
    cli
    mov ax, 0x23
    
    mov ds, ax

    mov es, ax

    mov fs, ax

    mov gs, ax

    push dword 0x23
    push dword [esp + 4]

    pushfd
    pop eax
    or eax, 0x200
    push eax

    push  dword 0x1B
    push dword [esp + 16]

    iretd