#include "tss.h"
#include "gdt.h"
#include "../alarm/panic.h"

struct tss_entry_t tss_entry;

extern void tss_flush(void);


static void write_tss(int gdt_index, uint32_t kernel_ss, uint32_t kernel_esp){

    for(uint32_t i = 0; i < sizeof(tss_entry); i++) {
        ((uint8_t*)&tss_entry)[i] = 0;
    }


    tss_entry.ss0 = kernel_ss;
    tss_entry.esp0 = kernel_esp;
    tss_entry.cs = 0x0B;
    tss_entry.ss = 0x13;
    tss_entry.ds = 0x13;
    tss_entry.es = 0x13;
    tss_entry.fs = 0x13;
    tss_entry.gs = 0x13;
    tss_entry.iomap_base = sizeof(tss_entry);

    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    gdt_set_gate(gdt_index, base, limit, 0x89, 0x00);

}



void tss_install(int gdt_index, uint32_t kernel_ss, uint32_t kernel_esp){
    write_tss(gdt_index, kernel_ss, kernel_esp);

    
   
    tss_flush();


    tss_self_test();

    
}

void set_kernel_stack(uint32_t stack){
    tss_entry.esp0 = stack;
}


void tss_self_test(void)
{
    if (tss_entry.esp0 == 0 || tss_entry.ss0 == 0)
        panic("TSS: esp0 or ss0 not initialized");

    if (tss_entry.cs != 0x0B || tss_entry.ss != 0x13)
        panic("TSS: Segment selectors incorrect");

    if (tss_entry.iomap_base != sizeof(tss_entry))
        panic("TSS: I/O map base incorrect");

    // Verify LTR
    uint16_t tr;
    asm volatile ("str %0" : "=r"(tr));
    if (tr != 0x28)
        panic("TSS: TR register not set properly");
}



