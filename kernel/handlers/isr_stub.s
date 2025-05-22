section .text


extern isr_divide_by_zero_stub_handler
extern isr_double_fault_stub_handler
extern isr_gpf_stub_handler
extern isr_page_fault_stub_handler
extern isr_generic_exception_stub_handler
extern syscall

global isr_divide_by_zero_stub
isr_divide_by_zero_stub:
    cli
    push dword 0              ; dummy error code (no error code for #DE)
    push dword 0              ; interrupt number 0
    call isr_divide_by_zero_stub_handler
    add esp, 8
    sti
    iret

global isr_double_fault_stub
isr_double_fault_stub:
    cli
    push dword [esp + 8]      ; push error code from stack
    push dword 8              ; interrupt number 8
    call isr_double_fault_stub_handler
    add esp, 8
    sti
    iret

global isr_gpf_stub
isr_gpf_stub:
    cli
    push dword [esp + 8]      ; push error code from stack
    push dword 13             ; interrupt number 13
    call isr_gpf_stub_handler
    add esp, 8
    sti
    iret

global isr_page_fault_stub
isr_page_fault_stub:
    cli
    push dword [esp + 8]      ; push error code from stack
    push dword 14             ; interrupt number 14
    call isr_page_fault_stub_handler
    add esp, 8
    sti
    iret

global isr_generic_exception_stub
isr_generic_exception_stub:
    cli
    push dword 0              ; dummy error code (no error code here)
    push dword 255            ; interrupt number 255 (example)
    call isr_generic_exception_stub_handler
    add esp, 8
    sti
    iret

global isr_syscall

isr_syscall:
    cli
    pusha
    call syscall
    popa
    iret


