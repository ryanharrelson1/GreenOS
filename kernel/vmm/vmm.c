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

typedef struct {
    vmm_region_t* free_list;     // freelist of free nodes
    vmm_region_t* pool_start;    // start of slab virtual address range
    uint32_t capacity;           // total nodes slab can hold
    uint32_t used;               // allocated nodes count
} vmm_region_slab_t;


static vmm_region_t* user_space_free_list = NULL;
static vmm_region_t* kernel_space_free_list = NULL;

static vmm_region_slab_t region_slab = {0};

static inline uint32_t align_up(uint32_t val) {
    return (val + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

void vmm_init(){

    static vmm_region_t user_init, kernel_init;

    user_init.start = USER_VIRT_START;
    user_init.size = USER_VIRT_END - USER_VIRT_START + 1;
    user_init.next = NULL;

    kernel_init.start = KERNEL_HEAP_START;
kernel_init.size = KERNEL_HEAP_END - KERNEL_HEAP_START + 1;
    kernel_init.next = NULL;

    user_space_free_list =&user_init;
    kernel_space_free_list =&kernel_init;


}

void vmm_region_slab_init() {
    if (region_slab.pool_start) return; // already init

    // Allocate and map physical pages for the slab
    uintptr_t vaddr = VMM_REGION_POOL_VADDR;
    for (int i = 0; i < VMM_REGION_POOL_PAGES; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) panic("Failed to allocate physical page for VMM region slab");
        uint32_t flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
               
        
        paging_map_page(vaddr + i * PAGE_SIZE, (uintptr_t)phys,flags);
        // flags set to present, writable, user/kernel as needed
    }

    region_slab.pool_start = (vmm_region_t*)vaddr;
    region_slab.capacity = (VMM_REGION_POOL_PAGES * PAGE_SIZE) / sizeof(vmm_region_t);
    region_slab.used = 0;

    // Initialize freelist: chain all nodes in slab as free
    for (uint32_t i = 0; i < region_slab.capacity - 1; i++) {
        region_slab.pool_start[i].next = &region_slab.pool_start[i + 1];
    }
    region_slab.pool_start[region_slab.capacity - 1].next = NULL;

    region_slab.free_list = region_slab.pool_start;
}


vmm_region_t* vmm_region_alloc() {
    if (!region_slab.pool_start) vmm_region_slab_init();

    if (!region_slab.free_list) {
        // No free nodes left, handle error or expand slab (not implemented here)
        return NULL;
    }

    vmm_region_t* node = region_slab.free_list;
    region_slab.free_list = node->next;
    region_slab.used++;

    // Zero node contents before use (optional)
    node->start = 0;
    node->size = 0;
    node->next = NULL;

    return node;
}

void vmm_region_free(vmm_region_t* node) {
    if (!node) return;

    // Zero memory (optional)
    node->start = 0;
    node->size = 0;

    node->next = region_slab.free_list;
    region_slab.free_list = node;
    region_slab.used--;
}

void* vmm_alloc(uint32_t size, bool kernel){
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

            if (curr->size == 0) {
            if (prev) prev->next = curr->next;
            else *list = curr->next;
             vmm_region_free(curr); // ✅ Free the used-up region back to slab
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

     // Allocate tracking node from slab
    vmm_region_t* node = vmm_region_alloc();
    if (!node) {
        panic("Out of VMM region slab nodes");
    }


    node->start = vaddr;
    node->size = size;

    vmm_region_t** list = kernel ? &kernel_space_free_list : &user_space_free_list;
    node->next = *list;
    *list = node;
}
