#ifndef VMM_H
#define VMM_H


#include <stddef.h>
#include "../stdint.h"


#define VMM_FLAG_RW   0x1
#define VMM_FLAG_USER 0x2
#define VMM_FLAG_EXEC 0x4



void vmm_init(void);


void* vmm_alloc(size_t size, uint32_t flags);

void vmm_free(void* addr);

int vmm_map(void* virt_addr, uintptr_t phys_addr, size_t size, uint32_t flags);

int vmm_unmap(void* virt_addr, size_t size);

void test_vmm();


#endif