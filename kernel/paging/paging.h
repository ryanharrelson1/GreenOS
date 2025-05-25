#ifndef PAGING_H
#define PAGING_H


#include "../stdint.h"
#include <stddef.h>

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024
#define TEMP_MAP_ADDR 0xF0000000


#define KERNEL_PHYS_WINDOW 0xC0000000
#define phys_to_virt(p) ((void*)((uintptr_t)(p) + KERNEL_PHYS_WINDOW))
#define virt_to_phys(v) ((uintptr_t)(v) - KERNEL_PHYS_WINDOW)


uintptr_t paging_init(uintptr_t identity_map_end);
void* phys_map(uintptr_t phys_addr);
void paging_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags);
void paging_unmap_page(uintptr_t virtual_addr);




void paging_run_tests();
void check_bitmap_mapped(uintptr_t virt_addr);





#endif