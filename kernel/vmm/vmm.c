#include "../memset.h"
#include "../paging/paging.h"
#include "vmm.h"
#include "../pmm/pmm.h"
#include"../consol/serial.h"
#include "../alarm/panic.h"




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
vmm_region_t* vmm_region_alloc() {
    if (!region_slab.pool_start) vmm_region_slab_init();

   if (!region_slab.free_list) {
        write_serial_string("[vmm] Alloc failed: Free list empty\n");
        return NULL;
    }

    vmm_region_t* node = region_slab.free_list;
    region_slab.free_list = node->next;
    region_slab.used++;


     write_serial_string("[vmm] Alloc region at: ");
    serial_write_hex32((uintptr_t)node);
    write_serial_string(" (used ");
    serial_write_hex32(region_slab.used);
    write_serial_string("/");
    serial_write_hex32(region_slab.capacity);
    write_serial_string(")\n");

    // Zero node contents before use (optional)
    node->start = 0;
    node->size = 0;
    node->next = NULL;

    return node;
}

void vmm_init() {
    vmm_region_slab_init(); // Ensure the slab is initialized before allocation

    vmm_region_t* user_init = vmm_region_alloc();
    if (!user_init) panic("Failed to allocate initial user region");

    user_init->start = USER_VIRT_START;
    user_init->size = USER_VIRT_END - USER_VIRT_START + 1;
    user_init->next = NULL;

    vmm_region_t* kernel_init = vmm_region_alloc();
    if (!kernel_init) panic("Failed to allocate initial kernel region");

    kernel_init->start = KERNEL_HEAP_START;
    kernel_init->size = KERNEL_HEAP_END - KERNEL_HEAP_START + 1;
    kernel_init->next = NULL;

    user_space_free_list = user_init;
    kernel_space_free_list = kernel_init;
}



void vmm_region_slab_init() {
   if (region_slab.pool_start) {
        write_serial_string("[vmm_region_slab_init] Slab already initialized\n");
        return;
    }

     write_serial_string("[vmm_region_slab_init] Initializing VMM region slab\n");

    // Allocate and map physical pages for the slab
    uintptr_t vaddr = VMM_REGION_POOL_VADDR;
    for (int i = 0; i < VMM_REGION_POOL_PAGES; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) panic("Failed to allocate physical page for VMM region slab");
        uint32_t flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
               
        
        paging_map_page(vaddr + i * PAGE_SIZE, (uintptr_t)phys,flags);

        write_serial_string("[vmm] Mapped page ");
        serial_write_hex32(i);
        write_serial_string(" -> vaddr: ");
        serial_write_hex32(vaddr + i * PAGE_SIZE);
        write_serial_string(" phys: ");
        serial_write_hex32((uintptr_t)phys);
        write_serial_string("\n");
        // flags set to present, writable, user/kernel as needed
    }

    region_slab.pool_start = (vmm_region_t*)vaddr;
    region_slab.capacity = (VMM_REGION_POOL_PAGES * PAGE_SIZE) / sizeof(vmm_region_t);
    region_slab.used = 0;

      write_serial_string("[vmm] Slab pool start: ");
      serial_write_hex32((uintptr_t)region_slab.pool_start);
        write_serial_string("\n");


    write_serial_string("[vmm] Slab capacity: ");
    serial_write_hex32(region_slab.capacity);
    write_serial_string(" regions, region size: ");
    serial_write_hex32(sizeof(vmm_region_t));
    write_serial_string("\n");


    // Initialize freelist: chain all nodes in slab as free
    for (uint32_t i = 0; i < region_slab.capacity - 1; i++) {
        region_slab.pool_start[i].next = &region_slab.pool_start[i + 1];
    }
    region_slab.pool_start[region_slab.capacity - 1].next = NULL;

    region_slab.free_list = region_slab.pool_start;

    int count = 0;
    vmm_region_t* node = region_slab.free_list;
    while (node) {
        count++;
        node = node->next;
    }

     write_serial_string("[vmm] Freelist initialized. Region count: ");
    serial_write_hex32(count);
    write_serial_string("\n");


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

void* vmm_alloc(uint32_t size, bool kernel) {
    write_serial_string("[vmm_alloc] Called with size: ");
    serial_write_hex32(size);
    write_serial_string(", kernel: ");
    write_serial_string(kernel ? "true\n" : "false\n");

    size = align_up(size);
    write_serial_string("[vmm_alloc] Aligned size: ");
    serial_write_hex32(size);
    write_serial_string("\n");

    vmm_region_t** list = kernel ? &kernel_space_free_list : &user_space_free_list;
    write_serial_string("[vmm_alloc] Using ");
    write_serial_string(kernel ? "kernel_space_free_list\n" : "user_space_free_list\n");

    vmm_region_t* curr = *list;
    vmm_region_t* prev = NULL;

    while (curr) {
        write_serial_string("[vmm_alloc] Inspecting region at ");
        serial_write_hex32(curr->start);
        write_serial_string(" size: ");
        serial_write_hex32(curr->size);
        write_serial_string("\n");

        if (curr->size >= size) {
            
            uint32_t result = curr->start;

            write_serial_string("[vmm_alloc] Found suitable region at ");
            serial_write_hex32(result);
            write_serial_string("\n");

            for (uint32_t offset = 0; offset < size; offset += PAGE_SIZE) {
                uint32_t phys = (uint32_t)pmm_alloc_page();
                if (!phys) {
                    write_serial_string("[vmm_alloc] pmm_alloc_page failed during mapping\n");
                    return NULL;
                }
                uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
                if (!kernel) flags |= PAGE_USER;

                write_serial_string("[paging_map_page] Mapping vaddr ");
                serial_write_hex32(result + offset);
                write_serial_string(" to phys ");
                serial_write_hex32(phys);
                write_serial_string(" with flags ");
                serial_write_hex32(flags);
                write_serial_string("\n");

                paging_map_page(result + offset, phys, flags);
                write_serial_string("retuned out of pageingmap ");

                // Zero the newly mapped page
                memset((void*)(result + offset), 0, PAGE_SIZE);
            }

            // Adjust free list region
            curr->start += size;
            curr->size -= size;

            if (curr->size == 0) {
                write_serial_string("[vmm_alloc] Region fully allocated, removing from free list\n");
                if (prev)
                    prev->next = curr->next;
                else
                    *list = curr->next;

                vmm_region_free(curr);
            }

            write_serial_string("[vmm_alloc] Allocation successful at ");
            serial_write_hex32(result);
            write_serial_string("\n");
            return (void*)result;
        }

        prev = curr;
        curr = curr->next;
    }

    write_serial_string("[vmm_alloc] No suitable region found, allocation failed\n");
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


void vmm_run_inline_tests() {
    write_serial_string("Running VMM inline tests...\n");

    // Init VMM

    // Test allocation
    void* ptr1 = vmm_alloc(8192, false); // 2 pages user
    if (!ptr1) panic("vmm_alloc failed on 2-page alloc");
    write_serial_string("Allocated 2 pages user.\n");

    void* ptr2 = vmm_alloc(4096, false); // 1 page user
    if (!ptr2) panic("vmm_alloc failed on 1-page alloc");
    write_serial_string("Allocated 1 page user.\n");

    // Check addresses are page aligned
    if ((uintptr_t)ptr1 % PAGE_SIZE != 0 || (uintptr_t)ptr2 % PAGE_SIZE != 0)
        panic("vmm_alloc returned unaligned address");
    write_serial_string("Allocation addresses are page-aligned.\n");

    // Free one
    vmm_free(ptr1, 8192, false);
    write_serial_string("Freed 2-page allocation.\n");

    // Free another
    vmm_free(ptr2, 4096, false);
    write_serial_string("Freed 1-page allocation.\n");

    // Reallocate and verify reuse
    void* ptr3 = vmm_alloc(4096, false);
    if (!ptr3) panic("vmm_alloc failed after free");
    write_serial_string("Reallocated 1 page after free.\n");

    write_serial_string("VMM inline tests passed.\n");
}
