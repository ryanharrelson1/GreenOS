#include "pmm.h"
#include "../alarm/panic.h"
#include "../consol/serial.h"

#define PAGE_SIZE 4096

extern uintptr_t _kernel_end ;
uint64_t bitmap_phys_start = 0;

uintptr_t bitmap_phys_end = 0;



 uint8_t* bitmap = 0;
 size_t bitmap_size = 0;
static uint64_t memory_start = 0;
static uint64_t memory_end = 0;
static size_t total_pages = 0;








static inline void bitmap_set(size_t index);
static inline void bitmap_clear(size_t index);
static inline int bitmap_test(size_t index);
static inline uintptr_t page_to_addr(size_t page);
static void pmm_self_test(void);




void pmm_init(struct mem_region* regions, size_t region_count){
 memory_start = UINT64_MAX;
 memory_end = 0;

 for (size_t i = 0; i< region_count; i++){
    if (regions[i].type != 1) continue;
    if (regions[i].base_addr < memory_start) memory_start = regions[i].base_addr;
    if (regions[i].base_addr + regions[i].length > memory_end)
    memory_end = regions[i].base_addr + regions[i].length;

 }

    total_pages = (memory_end - memory_start) / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;

    bitmap_phys_start = (_kernel_end + PAGE_SIZE -1) & ~(PAGE_SIZE - 1);
    bitmap_phys_end = bitmap_phys_start + bitmap_size;
    bitmap = (uint8_t*)bitmap_phys_start;

    memset(bitmap, 0xFF, bitmap_size);

    for(size_t i = 0; i < region_count; i++){
        if (regions[i].type != 1) continue;

        uint64_t base = regions[i].base_addr;
        uint64_t length = regions[i].length;

        for(uint64_t addr = base; addr < base + length; addr += PAGE_SIZE){
            if(addr< _kernel_end) continue;
            if(addr >= bitmap_phys_start && addr < (bitmap_phys_start + bitmap_size))
            continue;

            size_t page = (addr - memory_start) / PAGE_SIZE;
            bitmap_clear(page);
        }

    }
 

  bitmap_set(0);  


}


uintptr_t pmm_alloc_page(void) {
    write_serial_string("pmm_alloc_page: start\n");

    for (size_t i = 0; i < total_pages; i++) {
    
        if (!bitmap_test(i)) {
            write_serial_string("pmm_alloc_page: found free page index: ");
            serial_write_hex32((uint32_t)i);
            write_serial_string("\n");

            bitmap_set(i);

            uintptr_t addr = page_to_addr(i);
            write_serial_string("pmm_alloc_page: allocated page phys addr: 0x");
            serial_write_hex32((uint32_t)addr);
            write_serial_string("\n");

            write_serial_string("pmm_alloc_page: zeroing page memory\n");
            write_serial_string("pmm_alloc_page: memset done\n");

            return addr;
        }
    }

    write_serial_string("pmm_alloc_page: out of memory panic\n");
    panic("PMM: Out of physical memory!");
    return 0;
}



void pmm_free_page(uintptr_t  phys_addr) {
    if (phys_addr < memory_start || phys_addr >= memory_end) return;
    if (phys_addr % PAGE_SIZE != 0) return;  // Not page aligned
    size_t page = (phys_addr - memory_start) / PAGE_SIZE;
  
   bitmap_clear(page);
}

void pmm_mark_region_used(uintptr_t addr, size_t size) {
    if (addr < memory_start) addr = memory_start;
    if (addr + size > memory_end) size = memory_end - addr;

    for (uintptr_t a = addr; a < addr + size; a += PAGE_SIZE) {
        size_t page = (a - memory_start) / PAGE_SIZE;
        bitmap_set(page);
    }
}



size_t pmm_get_free_page_count(void) {
    size_t count = 0;
    for (size_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            count++;
        }
    }
    return count;
}


// --- Bitmap helpers ---
static inline void bitmap_set(size_t index) {
     if (index >= total_pages) return;  // Ignore out-of-range index
    bitmap[index / 8] |= (1 << (index % 8));
}

static inline void bitmap_clear(size_t index) {
     if (index >= total_pages) return;  // Ignore out-of-range index
    bitmap[index / 8] &= ~(1 << (index % 8));
}

static inline int bitmap_test(size_t index) {
     if (index >= total_pages) return 1; // Treat out-of-range as allocated/reserved
    return bitmap[index / 8] & (1 << (index % 8));
}

static inline uintptr_t page_to_addr(size_t page) {
    return memory_start + page * PAGE_SIZE;
}

size_t pmm_get_used_page_count(void) {
    return total_pages - pmm_get_free_page_count();
}

void pmm_print_total_memory(void) {
    uint64_t total_bytes = (memory_end - memory_start);

}

void pmm_print_free_memory(void) {
    size_t free_pages = pmm_get_free_page_count();
    uint64_t free_bytes = free_pages * PAGE_SIZE;

  
    
    // If you have a helper to print numbers in decimal or hex, use it here.
    // Otherwise, let's print hex as example:

    // Print hex representation (assuming write_serial_hex prints uint64_t in hex)


 
}


static inline void pmm_self_test(void) {
    size_t free_before = pmm_get_free_page_count();

    void* page = pmm_alloc_page();
    if (page == NULL) {
        panic("PMM self test failed: could not allocate page");
    }

    size_t free_after_alloc = pmm_get_free_page_count();
    if (free_after_alloc != free_before - 1) {
        panic("PMM self test failed: free page count did not decrease after alloc");
    }

    // Check that the allocated page is marked as used in bitmap
    uintptr_t addr = (uintptr_t)page;
    size_t page_index = (addr - memory_start) / PAGE_SIZE;
    if (!bitmap_test(page_index)) {
        panic("PMM self test failed: allocated page not marked as used");
    }

    pmm_free_page(addr);

    size_t free_after_free = pmm_get_free_page_count();
    if (free_after_free != free_before) {
        panic("PMM self test failed: free page count did not restore after free");
    }

    if (bitmap_test(page_index)) {
        panic("PMM self test failed: freed page still marked as used");
    }

    // Optional: Test allocating all pages (or some) then freeing

}


