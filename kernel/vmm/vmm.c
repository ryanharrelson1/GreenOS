#include <stddef.h>
#include <stdbool.h>
#include "stdint.h"
#include "../memory.h"
#include "../paging/paging.h"
#include "vmm.h"
#include "../pmm/pmm.h"


typedef struct vmm_region
{
    struct vmm_region* next;
    uint32_t start;
    uint32_t size;

 
} vmm_region_t;

static vmm_region_t* user_space_free_list = NULL;
static vmm_region_t* kernel_space_free_list = NULL;

static inline uint32_t align_up(uint32_t val) {
    return (val + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

void vmm_init(){

    static vmm_region_t user_init, kernel_init;

    user_init.start = USER_VIRT_START;
    user_init.size = USER_VIRT_END - USER_VIRT_START + 1;
    user_init.next = NULL;

    kernel_init.start = USER_VIRT_START;
    kernel_init.size = USER_VIRT_END - USER_VIRT_START + 1;
    kernel_init.next = NULL;

    user_space_free_list =&user_init;
    kernel_space_free_list =&kernel_init;


}

void vmm_alloc(uint32_t size, bool kernel){
    size = align_up(size);
    vmm_region_t** list = kernel ? &kernel_space_free_list : &user_space_free_list;
    vmm_region_t* curr = *list;
    vmm_region_t* prev = NULL;


    while(curr){
        if(curr->size >=size){

            uint32_t result = curr->start;


            for(uint32_t offset = 0; offset < size; offset += PAGE_SIZE){
                uint32_t phys = (uint32_t)pmm_alloc_page();
                uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
                if (!kernel) flags |= PAGE_USER;
                paging_map_page(result + offset, phys, flags);
                memset((void*)(result + offset), 0, PAGE_SIZE);
            }

            curr->start += size;
            curr->size -= size;

            if(size == 0){
                if(prev) prev->next;
                else *list = curr->next;
            }

            return(void*)result;
        }
        prev = curr;
        curr = curr->next;

    }
    return NULL;

}


void vmm_free(void* addr, uint32_t size, bool kernel) {
    size = align_up(size);
    uintptr_t vaddr = (uintptr_t)addr;

    // Unmap all pages
    for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
        paging_unmap_page(vaddr + offset);
    }

    // Add the region to the free list
    vmm_region_t* node = (vmm_region_t*)pmm_alloc_page(); // consider slab later
    node->start = vaddr;
    node->size = size;

    vmm_region_t** list = kernel ? &kernel_space_free_list : &user_space_free_list;
    node->next = *list;
    *list = node;
}
