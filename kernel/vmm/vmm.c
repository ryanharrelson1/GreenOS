#include "vmm.h"
#include "../paging/paging.h"
#include "../pmm/pmm.h"



#define VMM_BASE  0x10000000
#define VMM_LIMIT   0xF0000000
#define PAGE_SIZE 0x1000


typedef struct vmm_region
{
    uintptr_t start;
    size_t size;
    uint32_t flags;
    struct vmm_region* next;
  
    
} vmm_region_t;

static vmm_region_t* vmm_head = NULL;

void vmm_init(void) {

    vmm_head = NULL;
}

static int is_overlap(uintptr_t a_start, size_t a_size, uintptr_t b_start, size_t b_size) {
    return (a_start < b_start + b_size) && (b_start < a_start + a_size);
}

void* vmm_alloc(size_t size, uint32_t flags){

    size = (size + PAGE_SIZE - 1)& ~(PAGE_SIZE - 1);

    uintptr_t addr = VMM_BASE;
    vmm_region_t* prev = NULL;
    vmm_region_t* current = vmm_head;

    while(current){
        if(addr + size <= current->start) break;
        addr = current->start + current->size;
        prev = current;
        current = current->next;
    }

    if(addr + size > VMM_LIMIT)return;

    vmm_region_t* region = (vmm_region_t*)pmm_alloc_page();
     if (!region) return NULL;

     region->start = addr;
     region->size = size;
     region->flags = flags;
     region->next = current;

     if(prev){
        prev->next = region;
     } else {
        vmm_head = region;
     }

      return (void*)addr;
}

void vmm_free(void* addr){

    uintptr_t target = (uintptr_t)addr;

    vmm_region_t* prev = NULL;
    vmm_region_t* current = vmm_head;

     while (current) {
        if (current->start == target) {
            if (prev) {
                prev->next = current->next;
            } else {
                vmm_head = current->next;
            }
            pmm_free((void*)current);
            return;
        }
       prev = current;
       current = current->next; 
}
}

int vmm_map(void* virt_addr, uintptr_t phys_addr, size_t size, uint32_t flags) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uintptr_t va = (uintptr_t)virt_addr;

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        if (!paging_map_page(va + i, phys_addr + i, flags)) {
            return -1;
        }
    }
    return 0;
}

int vmm_unmap(void* virt_addr, size_t size) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uintptr_t va = (uintptr_t)virt_addr;

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        paging_unmap_page(va + i);
    }
    return 0;
}

