#include "handler_init.h"
#include "../alarm/panic.h"
#include "../idt/idt.h"

extern void isr_divide_by_zero_stub(void);
extern void isr_double_fault_stub(void);
extern void isr_gpf_stub(void);
extern void isr_page_fault_stub(void);
extern void isr_generic_exception_stub(void);




void handlers_install(void){

    idt_set_gate(0, (uint32_t)isr_divide_by_zero_stub, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr_double_fault_stub, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr_gpf_stub, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr_page_fault_stub, 0x08, 0x8E);


    for (int i = 0; i < 256; i++) {
        if (i != 0 && i != 8 && i != 13 && i != 14) {
            idt_set_gate(i, (uint32_t)isr_generic_exception_stub, 0x08, 0x8E);
        }
    }

}


