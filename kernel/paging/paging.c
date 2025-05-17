#include "paging.h"
#include "../pmm/pmm.h"
#include <stdbool.h>
#include "../memset.h"
#include "../alarm/panic.h"
#include "../consol/serial.h"


static uint32_t* current_directory;

static uint32_t paging_map_count = 0;

static void paging_test_map_unmap();


void serial_write_paging_map_info(uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags) {
    paging_map_count++;

    write_serial_string("paging_map #");
    serial_write_dec(paging_map_count);
    write_serial_string(": virt=");
    serial_write_hex64(virt_addr);
    write_serial_string(" -> phys=");
    serial_write_hex64(phys_addr);
    write_serial_string(", flags=");
    serial_write_hex32(flags);
    write_serial('\n');
}



#define ENTRIES_PER_TABLE 1024
#define PAGE_TABLE_SIZE (ENTRIES_PER_TABLE * sizeof(uint32_t))

extern uint32_t _kernel_end ;

extern uintptr_t kernel_start;


static inline void load_page_directory(uint32_t* dir) {
    __asm__ volatile ("mov %0, %%cr3" :: "r"(dir));
}


static inline void enable_paging() {
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set PG bit
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0));
}


static uint32_t* alloc_page_table() {
    uint32_t frame = pmm_alloc_page();
    uint32_t* table = (uint32_t*)(frame);
    memset(table, 0, PAGE_TABLE_SIZE);
    return table;
}


static void map_page(uint32_t* dir, uintptr_t virt, uintptr_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(dir[pd_idx] & PAGE_PRESENT)) {
        uint32_t* pt = alloc_page_table();
        dir[pd_idx] = ((uintptr_t)pt) | flags | PAGE_PRESENT;
    }

    uint32_t* pt = (uint32_t*)(dir[pd_idx] & 0xFFFFF000);
    pt[pt_idx] = (phys & 0xFFFFF000) | flags | PAGE_PRESENT;

}


void* paging_map(uintptr_t virt_addr, uint32_t flags) {
    uintptr_t phys = (uintptr_t)pmm_alloc_page();
    if (phys == 0) {
        panic("paging_map: Out of physical memory!\n");
    }

    serial_write_paging_map_info(virt_addr, phys, flags);

    map_page(current_directory, virt_addr, phys, flags);
    return (void*)virt_addr;
}
void paging_ident_map(uintptr_t addr, size_t size, uint32_t flags) {
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        map_page(current_directory, addr + i, addr + i, flags);
    }
}


void paging_map_kernel(uint32_t kernel_start, uint32_t kernel_end) {
    for (uintptr_t addr = kernel_start; addr < _kernel_end; addr += PAGE_SIZE) {
        map_page(current_directory, addr, addr, PAGE_RW);
    }
}

void paging_switch_dir(uint32_t* new_directory) {
    current_directory = new_directory;
    load_page_directory(current_directory);
}

uint32_t* paging_current_dir() {
    return current_directory;
}


uint32_t* paging_create_user_directory() {
    uint32_t* new_dir = alloc_page_table();

    // Copy kernel space mappings (768 to 1023)
    for (int i = 768; i < 1024; i++) {
        new_dir[i] = current_directory[i];
    }

    // Add recursive mapping
    new_dir[1023] = (uintptr_t)new_dir | PAGE_RW | PAGE_PRESENT;

    return new_dir;
}

void* paging_map_user(uint32_t* dir, uintptr_t virt_addr, uint32_t flags) {
    uintptr_t phys = pmm_alloc_page();
    map_page(dir, virt_addr, phys, flags | PAGE_USER);
    return (void*)virt_addr;
}


void paging_unmap(uint32_t* dir, uint32_t virt_addr) {
    uint32_t pd_idx = virt_addr >> 22;
    uint32_t pt_idx = (virt_addr >> 12) & 0x3FF;

    if (!(dir[pd_idx] & PAGE_PRESENT)) return; // No page table

    uint32_t* pt = (uint32_t*)(dir[pd_idx] & 0xFFFFF000);
    uint32_t pte = pt[pt_idx];

    if (!(pte & PAGE_PRESENT)) return; // No mapped page

    // Free physical frame
    uint32_t phys_addr = pte & 0xFFFFF000;
    pmm_free_page(phys_addr);

    // Clear the page table entry
    pt[pt_idx] = 0;

    // Invalidate TLB for this address
    __asm__ volatile ("invlpg (%0)" :: "r" (virt_addr) : "memory");

    // Optional: Check if page table is empty and free it if so
    bool empty = true;
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        if (pt[i] & PAGE_PRESENT) {
            empty = false;
            break;
        }
    }

    if (empty) {
        // Free the page table frame itself
        uint32_t pt_phys = (uint32_t)pt & 0xFFFFF000;
        pmm_free_page(pt_phys);

        // Clear page directory entry
        dir[pd_idx] = 0;

        // Invalidate TLB for page directory entry
        __asm__ volatile ("invlpg (%0)" :: "r" (virt_addr & 0xFFC00000) : "memory");
    }
}


void paging_init() {
    current_directory = alloc_page_table(); // Allocated and zeroed

    // Map the kernel 1:1
    extern uint32_t kernel_start;
    extern uint32_t _kernel_end;
    paging_map_kernel((uintptr_t)&kernel_start, (uintptr_t)&_kernel_end);

    // Map VGA text buffer
    paging_ident_map(0xB8000, PAGE_SIZE, PAGE_RW);

    // Recursive map for user/kernel space to easily access page tables
    current_directory[1023] = (uintptr_t)current_directory | PAGE_RW | PAGE_PRESENT;

    load_page_directory(current_directory);
    enable_paging();
    paging_test_map_unmap();
}



static void paging_test_map_unmap() {
    uintptr_t test_virt = 0x400000; // 4MB aligned test address

    // Map a page with RW permissions
    void* mapped = paging_map(test_virt, PAGE_RW);
    if (mapped != (void*)test_virt) {
        write_serial_string("Test map failed: returned address mismatch\n");
        return;
    }
    write_serial_string("Test map succeeded\n");

    // Try to map again to see if it allocates a new physical page or errors
    void* mapped2 = paging_map(test_virt + PAGE_SIZE, PAGE_RW);
    if (mapped2 == NULL) {
        write_serial_string("Test map2 failed: returned NULL\n");
        return;
    }
    write_serial_string("Test map2 succeeded\n");

    // Unmap the first page
    paging_unmap(current_directory, test_virt);
    write_serial_string("Test unmap done\n");

    // Unmap the second page
    paging_unmap(current_directory, test_virt + PAGE_SIZE);
    write_serial_string("Test unmap2 done\n");
}
