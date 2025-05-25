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
#include "usermode/user.h"
#include "pmm/pmm.h"
#include "vmm/vmm.h"


extern uint32_t stack_top;
 extern uint64_t bitmap_phys_start;
 extern uint8_t* bitmap;
 extern size_t bitmap_size;
 extern uintptr_t _kernel_end;

 void test_bitmap_identity_mapping() {
    write_serial_string("Test bitmap identity mapping\n");

    // Print physical address
    write_serial_string("Physical bitmap address: 0x");
    serial_write_hex64(bitmap_phys_start);
    write_serial_string("\n");

    // Print virtual address (bitmap pointer)
    write_serial_string("Virtual bitmap address: ");
    serial_write_hex64((uintptr_t)bitmap);
    write_serial_string("\n");

    if ((uintptr_t)bitmap == bitmap_phys_start) {
        write_serial_string("Bitmap is identity mapped!\n");
    } else {
        write_serial_string("Bitmap is NOT identity mapped!\n");
    }

    // Optional: Try reading/writing bitmap data safely
    if (bitmap_size > 0) {
        write_serial_string("Bitmap first byte before: 0x");
        serial_write_hex32(bitmap[0]);
        write_serial_string("\n");

        // Flip first bit of first byte as test
        bitmap[0] ^= 0x01;

        write_serial_string("Bitmap first byte after toggle: 0x");
        serial_write_hex32(bitmap[0]);
        write_serial_string("\n");

        // Restore original bit
        bitmap[0] ^= 0x01;
    }
}




void kernel_main(uintptr_t  mb_info_addr) {
    init_serial();
    gdt_install();
    tss_install(5, 0x10, (uint32_t)&stack_top);
    idt_install();
    pic_remap();
    handlers_install();
    parse_memory_map(mb_info_addr);

    uintptr_t paging_region_start = bitmap_phys_end; 
    
   uintptr_t physical_end = paging_init(bitmap_phys_end);

   pmm_mark_region_used(paging_region_start, physical_end - paging_region_start);
   vmm_init();


     uintptr_t phys = pmm_alloc_page();
    if (!phys) {
        write_serial_string("PMM alloc failed!\n");
        return;
    }

    write_serial_string("Allocated physical page at: ");
    serial_write_hex32((uint32_t)phys);
    write_serial_string("\n");

    void* temp = vmm_temp_map(phys);
    if (!temp) {
        write_serial_string("vmm_temp_map failed!\n");
        return;
    }

    write_serial_string("Mapped to temp addr: ");
    serial_write_hex32((uint32_t)(uintptr_t)temp);
    write_serial_string("\n");

    *((volatile uint32_t*)temp) = 0xDEADBEEF;
    if (*((volatile uint32_t*)temp) == 0xDEADBEEF) {
        write_serial_string("Temp map memory write/read OK\n");
    } else {
        write_serial_string("Temp map memory write/read FAIL\n");
    }


    write_serial_string("done it wokred fuck me\n");
   



   

  


   
   


   serial_write_hex32((uint32_t)physical_end);
   write_serial_string("\n");   // value it holds

  serial_write_hex32(bitmap_phys_start);  
     

   //paging_run_tests();
   //test_vmm();

  
 
   

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
};

void trigger_page_fault() {
    volatile int *ptr = (int *)0xDEADBEEF; // or (int *)0
    *ptr = 42; // writing to invalid memory triggers page fault
}

void trigger_gpf() {
    asm volatile ("int $13"); // explicitly trigger interrupt 13
}