#ifndef PAGING_H
#define PAGING_H


#include "../stdint.h"
#include <stddef.h>

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024


uintptr_t paging_init(uintptr_t identity_map_end);




void paging_run_tests();
void check_bitmap_mapped(uintptr_t virt_addr);




#endif