#include "paging.h"
#include "../memory_map.h"
#include "../alarm/panic.h"
#include "../consol/serial.h"
#include "../pmm/pmm.h"





#define PDE_PRESENT 0x1
#define PDE_RW 0x2
#define PDE_USER 0x4

#define PTE_PRESENT 0x1
#define PTE_RW 0x2
#define PTE_USER 0x4
#define ALIGN_UP(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define TEMP_VIRT_ADDR 0xCAFEB000 



// these are for init only do not use
static uint32_t* page_directory;
static uint32_t* page_tables;

// Helper: flush TLB for a single page
static inline void flush_tlb_single(uintptr_t addr) {
    write_serial_string("[flush_tlb_single] Flushing TLB for addr: 0x");
    serial_write_hex32((uint32_t)addr);
      write_serial_string("\n");
    __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
}







void map_physical_memory_window(uintptr_t max_phys) {
    for (uintptr_t phys = 0; phys < max_phys; phys += PAGE_SIZE) {
        uintptr_t virt = KERNEL_PHYS_WINDOW + phys;

        uint32_t pd_index = (virt >> 22) & 0x3FF;
        uint32_t pt_index = (virt >> 12) & 0x3FF;

        if (!(page_directory[pd_index] & PDE_PRESENT)) {
            uint32_t* new_pt = (uint32_t*)pmm_alloc_page();
            if (!new_pt) panic("Out of memory for PT");

            memset(new_pt, 0, PAGE_SIZE);
            page_directory[pd_index] = ((uintptr_t)new_pt) | PDE_PRESENT | PDE_RW;
        }

        uint32_t* page_table = (uint32_t*)(page_directory[pd_index] & ~0xFFF);
        page_table[pt_index] = (phys & ~0xFFF) | PTE_PRESENT | PTE_RW;
    }
}

uint32_t* get_page_table_virt(uint32_t pd_index) {
    return (uint32_t*)(RECURSIVE_BASE_VADDR + (pd_index * PAGE_SIZE));
}


void paging_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags){
     write_serial_string("[paging_map_page] Called with virt=0x");
    serial_write_hex32((uint32_t)virt);
    write_serial_string(", phys=0x");
    serial_write_hex32((uint32_t)phys);
    write_serial_string(", flags=0x");
    serial_write_hex32(flags);
    write_serial_string("\n");


    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    write_serial_string("[paging_map_page] Calculated pd_index=0x");
    serial_write_hex32(pd_index);
    write_serial_string(", pt_index=0x");
    serial_write_hex32(pt_index);
    write_serial_string("\n");


    uint32_t pd_entry = page_directory[pd_index];
    write_serial_string("[paging_map_page] PDE value: 0x");
    serial_write_hex32(pd_entry);
    write_serial_string("\n");



    if(!(pd_entry & PDE_PRESENT)){
         write_serial_string("[paging_map_page] PDE not present, allocating new page table\n");
      uint32_t pt_phys = pmm_alloc_page(); 
        if (!pt_phys ) {
      panic("Out of memory: failed to allocate page table");
     }

   
    

        
         write_serial_string("[paging_map_page] New PT phys addr: 0x");
        serial_write_hex32((uint32_t)pt_phys);
        write_serial_string("\n");
        page_directory[pd_index] = pt_phys | PDE_PRESENT | PDE_RW | PDE_USER;
        flush_tlb_single(0xFFFFF000 + (pd_index * PAGE_SIZE)); 


        uint32_t* pt_virt = get_page_table_virt(pd_index);
        memset(pt_virt, 0, PAGE_SIZE);


         write_serial_string("[paging_map_page] Updated PDE at index ");
        serial_write_hex32(pd_index);
        write_serial_string(" to 0x");
        serial_write_hex32(page_directory[pd_index]);
        write_serial_string("\n");
    
    }

    uint32_t* page_table = get_page_table_virt(pd_index);
     write_serial_string("[paging_map_page] Page table address: 0x");
    serial_write_hex32((uint32_t)page_table);
    write_serial_string("\n");

     write_serial_string("[paging_map_page] Setting PTE at index ");
    serial_write_hex32(pt_index);
    write_serial_string(" to phys addr 0x");
    serial_write_hex32((phys & ~0xFFF));
    write_serial_string(" with flags 0x");
    serial_write_hex32(flags & 0xFFF);
    write_serial_string("\n");

    page_table[pt_index] = (phys & ~0xFFF) | (flags & 0xFFF) | PTE_PRESENT;

    flush_tlb_single(virt);
}

void paging_unmap_page(uintptr_t virtual_addr) {

    write_serial_string("[paging_unmap_page] Called with virtual_addr=0x");
    serial_write_hex32((uint32_t)virtual_addr);
    write_serial_string("\n");

    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

     write_serial_string("[paging_unmap_page] pd_index=0x");
    serial_write_hex32(pd_index);
    write_serial_string(", pt_index=0x");
    serial_write_hex32(pt_index);
    write_serial_string("\n");

    if (!(page_directory[pd_index] & PDE_PRESENT)) {
        write_serial_string("[paging_unmap_page] PDE not present, nothing to unmap\n");
        return; // Page table not present
    }

    uint32_t* pt = (uint32_t*)(page_directory[pd_index] & ~0xFFF);

    write_serial_string("[paging_unmap_page] Page table address: 0x");
    serial_write_hex32((uint32_t)pt);
    write_serial_string("\n");
    uint32_t entry = pt[pt_index];

    write_serial_string("[paging_unmap_page] PTE value: 0x");
    serial_write_hex32(entry);
    write_serial_string("\n");


    if (!(entry & PTE_PRESENT)) {
         write_serial_string("[paging_unmap_page] PTE not present, nothing to unmap\n");
        return; // Page not mapped
    }

    uintptr_t phys_addr = entry & ~0xFFF;
    write_serial_string("[paging_unmap_page] Freeing physical page at 0x");
    serial_write_hex32((uint32_t)phys_addr);
    write_serial_string("\n");

    pmm_free_page((void*)phys_addr);  // Free the physical frame

    pt[pt_index] = 0; // Clear the entry

    flush_tlb_single(virtual_addr);
}


uintptr_t paging_init(uintptr_t identity_map_end) {
    write_serial_string("[paging_init] Called with identity_map_end=");
      serial_write_hex32((uint32_t)identity_map_end);
    write_serial_string("\n");

    identity_map_end = ALIGN_UP(identity_map_end, PAGE_SIZE);
     write_serial_string("[paging_init] Aligned identity_map_end=0x");
    serial_write_hex32((uint32_t)identity_map_end);
    write_serial_string("\n");


    // these are for paging init only do not use 
   uint32_t temp_pages = identity_map_end / PAGE_SIZE;
    uint32_t tables_needed = (temp_pages + PAGE_ENTRIES - 1) / PAGE_ENTRIES;

    write_serial_string("[paging_init] total_pages=0x");
    serial_write_hex32(temp_pages);
    write_serial_string(", tables_needed=0x");
    serial_write_hex32(tables_needed);
    write_serial_string("\n");


    // Allocate page tables and directory after the identity-mapped region
    uintptr_t page_tables_start = identity_map_end;
    uintptr_t page_directory_start = page_tables_start + tables_needed * PAGE_SIZE;
    uintptr_t final_end = page_directory_start + PAGE_SIZE;

    uint32_t total_pages = final_end / PAGE_SIZE;

    write_serial_string("[paging_init] page_tables_start=0x");
    serial_write_hex32((uint32_t)page_tables_start);
    write_serial_string(", page_directory_start=0x");
    serial_write_hex32((uint32_t)page_directory_start);
    write_serial_string(", final_end=0x");
    serial_write_hex32((uint32_t)final_end);
    write_serial_string("\n");

    page_tables = (uint32_t*)page_tables_start;
    page_directory = (uint32_t*)page_directory_start;

     write_serial_string("[paging_init] Clearing page tables and directory memory\n");
    memset(page_tables, 0, tables_needed * PAGE_SIZE);
    memset(page_directory, 0, PAGE_SIZE);
write_serial_string("[paging_init] Identity mapping pages...\n");
    // Identity map all pages up to final_end
    for (uint32_t page_idx = 0; page_idx < total_pages; page_idx++) {
        uint32_t table_idx = page_idx / PAGE_ENTRIES;
        uint32_t entry_idx = page_idx % PAGE_ENTRIES;

        if (entry_idx == 0) {
            // Point page directory entry to this page table
            page_directory[table_idx] = ((uintptr_t)&page_tables[table_idx * PAGE_ENTRIES]) | PDE_PRESENT | PDE_RW;

            write_serial_string("[paging_init] PDE set at index ");
            serial_write_hex32(table_idx);
            write_serial_string(" to 0x");
            serial_write_hex32(page_directory[table_idx]);
            write_serial_string("\n");
        }

      

        page_tables[page_idx] = (page_idx * PAGE_SIZE) | PTE_PRESENT | PTE_RW;


         write_serial_string("[paging_init] PTE set at index ");
        serial_write_hex32(page_idx);
        write_serial_string(" to 0x");
        serial_write_hex32(page_tables[page_idx]);
        write_serial_string("\n");
    } 

    page_directory[1023] = ((uintptr_t)page_directory) | PDE_PRESENT | PDE_RW;

    write_serial_string("[paging_init] Set recursive mapping at PDE index 1023\n");
    
    map_physical_memory_window(final_end);
 
    // Load CR3 and enable paging
    __asm__ __volatile__ (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        :
        : "r"(page_directory)
        : "eax"
    );
       

     write_serial_string("[paging_init] Loading CR3 and enabling paging\n");

  uintptr_t phys = page_directory_start;
   uint32_t dir_idx = phys >> 22;

write_serial_string("Checking if page directory is mapped at dir_idx: ");
serial_write_hex32(dir_idx);
write_serial_string("\n");

if (page_directory[dir_idx] & PDE_PRESENT) {
    write_serial_string("Page directory is mapped!\n");
} else {
    write_serial_string("Page directory is NOT mapped!\n");
}
    return final_end;
}




void paging_run_tests() {

     

  write_serial_string("Page directory first entry: 0x");
serial_write_hex32(((uint32_t*)0x3A547000)[0]);
write_serial_string("\n");

    // Step 1: Allocate a physical page
    uintptr_t phys_addr = (uintptr_t)pmm_alloc_page();
    if (!phys_addr) {
        panic("Test failed: couldn't allocate physical page");
    }

    uintptr_t test_virt = 0x40000000; // Arbitrary virtual address
     write_serial_string("mapping\n");
    // Step 2: Map the page
    paging_map_page(test_virt, (uintptr_t)phys_addr, PTE_RW | PTE_USER);

    write_serial_string("Mapped virtual address: 0x");
    serial_write_hex32((uint32_t)test_virt);
    write_serial_string(" to physical: 0x");
    serial_write_hex32((uint32_t)phys_addr);
    write_serial_string("\n");

    // Step 3: Write and verify
    volatile uint32_t* test_ptr = (uint32_t*)test_virt;
    *test_ptr = 0x12345678;

    if (*test_ptr != 0x12345678) {
        panic("Test failed: memory write/read mismatch");
    }

    // Step 4: Unmap
    paging_unmap_page(test_virt);

    write_serial_string("Unmapped virtual address: 0x");
    serial_write_hex32((uint32_t)test_virt);
    write_serial_string("\n");

    write_serial_string("Paging tests passed.\n");
}

