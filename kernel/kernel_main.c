#include "consol/serial.h"
#include "alarm/panic.h"
#include "gdt/gdt.h"
#include "gdt/tss.h"
#include "stdint.h"
#include "idt/idt.h"
#include "pic/pic.h"
#include "handlers/handler_init.h"
#include "memory_map.h"
#include "paging/paging.h"


extern uint32_t stack_top;



void kernel_main(uintptr_t  mb_info_addr) {
    init_serial();
    gdt_install();
    tss_install(5, 0x10, (uint32_t)&stack_top);
    idt_install();
    pic_remap();
    handlers_install();
    parse_memory_map(mb_info_addr);
    paging_init();


   
    

 asm volatile("sti");

 

    while (1);
}


__attribute__((naked)) void trigger_invalid_opcode() {
    __asm__ volatile (
        "ud2"  // Invalid instruction; causes #UD
    );
}

void trigger_divide_by_zero() {
    int a = 1;
    int b = 0;
    int c = a / b; // This will trigger interrupt 0
    (void)c;       // Prevent unused variable warning
}

void trigger_page_fault() {
    volatile int *ptr = (int *)0xDEADBEEF; // or (int *)0
    *ptr = 42; // writing to invalid memory triggers page fault
}

void trigger_gpf() {
    asm volatile ("int $13"); // explicitly trigger interrupt 13
}