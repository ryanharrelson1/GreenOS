#include "stdint.h"
#include "memory_map.h"
#include <stddef.h>


#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024
#define PDE_PRESENT 0x1
#define PDE_RW 0x2


#define PTE_PRESENT 0x1
#define PTE_RW 0x2

#define ALIGN_UP(x, a) (((x) + ((a)-1)) & ~((a)-1))

static uint32_t* page_directory;
static uint32_t* page_tables;

__attribute__((section(".text.stub")))
void setup(uintptr_t  mb_info_addr){
   
 parse_memory_map(mb_info_addr);



    











}


void early_paging(uintptr_t identity_map_end ){

        identity_map_end = ALIGN_UP(identity_map_end, PAGE_SIZE);

    // these are for paging init only do not use 
   uint32_t temp_pages = identity_map_end / PAGE_SIZE;
    uint32_t tables_needed = (temp_pages + PAGE_ENTRIES - 1) / PAGE_ENTRIES;


    // Allocate page tables and directory after the identity-mapped region
    uintptr_t page_tables_start = identity_map_end;
    uintptr_t page_directory_start = page_tables_start + tables_needed * PAGE_SIZE;
    uintptr_t final_end = page_directory_start + PAGE_SIZE;

    uint32_t total_pages = final_end / PAGE_SIZE;


    page_tables = (uint32_t*)page_tables_start;
    page_directory = (uint32_t*)page_directory_start;

    memset(page_tables, 0, tables_needed * PAGE_SIZE);
    memset(page_directory, 0, PAGE_SIZE);
    // Identity map all pages up to final_end
    for (uint32_t page_idx = 0; page_idx < total_pages; page_idx++) {
        uint32_t table_idx = page_idx / PAGE_ENTRIES;
        uint32_t entry_idx = page_idx % PAGE_ENTRIES;

        if (entry_idx == 0) {
            // Point page directory entry to this page table
            page_directory[table_idx] = ((uintptr_t)&page_tables[table_idx * PAGE_ENTRIES]) | PDE_PRESENT | PDE_RW;

        }


        page_tables[page_idx] = (page_idx * PAGE_SIZE) | PTE_PRESENT | PTE_RW;


    } 

    page_directory[1023] = ((uintptr_t)page_directory) | PDE_PRESENT | PDE_RW;

    
 
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
    
  uintptr_t phys = page_directory_start;
   uint32_t dir_idx = phys >> 22;



    return final_end;




}
__attribute__((section(".text.stub")))
void early_pmm_init(struct mem_region* regions, size_t region_count){

         volatile char* vga = (volatile char*) 0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i * 2] = 'b';
        vga[i * 2 + 1] = 0x1F; // White on blue
    }



}



