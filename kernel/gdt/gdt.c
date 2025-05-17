#include "gdt.h"
#include "../alarm/panic.h"


struct gdt_entry_t gdt_entries[6];
struct gdt_ptr_t gdt_ptr;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) 
{
  gdt_entries[num].base_low =(base & 0xFFFF);
  gdt_entries[num].base_middle = (base >> 16) & 0xFF;
  gdt_entries[num].base_high = (base >> 24) & 0xFF;

  gdt_entries[num].limit_low = (limit & 0xFFFF);
  gdt_entries[num].granularity = ((limit >> 16) & 0x0F);

  gdt_entries[num].granularity |= (gran & 0xF0);
  gdt_entries[num].access = access;

}

extern void gdt_flush(uint32_t);

void gdt_install(void)
{
 gdt_ptr.limit = (sizeof(struct gdt_entry_t) * 6) - 1;
 gdt_ptr.base = (uint32_t)&gdt_entries;

 gdt_set_gate(0, 0, 0, 0, 0);

 gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

 gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

 gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

 gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

 gdt_flush((uint32_t)&gdt_ptr);

 gdt_self_test();

}


void gdt_self_test(void)
{
    if (gdt_ptr.limit != (sizeof(struct gdt_entry_t) * 6 - 1)) {
        panic("GDT: Limit mismatch");
    }

    if (gdt_ptr.base != (uint32_t)&gdt_entries) {
        panic("GDT: Base mismatch");
    }

    uint16_t cs;
    asm volatile ("mov %%cs, %0" : "=r"(cs));
    if (cs != 0x08) {
        panic("GDT: Code segment not loaded (expected 0x08)");
    }


     if ((gdt_entries[3].access & 0x60) != 0x60 || !(gdt_entries[3].access & 0x10)) {
        panic("GDT: User code segment invalid");
    }

    if ((gdt_entries[4].access & 0x60) != 0x60 || !(gdt_entries[4].access & 0x10)) {
        panic("GDT: User data segment invalid");
    }

    // Optional: Check that the base and limit are what we expect (0 and 0xFFFFFFFF)
    if (gdt_entries[3].base_low != 0 || gdt_entries[3].base_middle != 0 || gdt_entries[3].base_high != 0) {
        panic("GDT: User code base is not 0");
    }

    if (gdt_entries[4].base_low != 0 || gdt_entries[4].base_middle != 0 || gdt_entries[4].base_high != 0) {
        panic("GDT: User data base is not 0");
    }

    // Add more segment checks if needed (ds, ss, etc)
}



