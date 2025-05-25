#ifndef VMM_H
#define VMM_H

#define PAGE_SIZE 4096
#define USER_VIRT_START     0x00100000   // Skip first 1MB
#define USER_VIRT_END       0xBFFFFFFF   // End before kernel

#define KERNEL_HEAP_START   0xC1000000
#define KERNEL_HEAP_END     0xE0000000
#define PAGE_PRESENT  0x1
#define PAGE_WRITE    0x2
#define PAGE_USER     0x4
#define VMM_REGION_POOL_VADDR 0xFFFFF00000000000UL  // example: kernel high half unused region
#define VMM_REGION_POOL_PAGES 4                     // 4 pages => ~16KB slab

#endif



