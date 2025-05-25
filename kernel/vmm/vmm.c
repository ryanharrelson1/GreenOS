#include "vmm.h"
#include "../paging/paging.h"
#include "../pmm/pmm.h"
#include "../consol/serial.h"



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

void* vmm_alloc(size_t size, uint32_t flags) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uintptr_t addr = VMM_BASE;
    vmm_region_t* prev = NULL;
    vmm_region_t* current = vmm_head;

    write_serial_string("vmm_alloc start\n");
    write_serial_string("Requested size: ");
    serial_write_dec((int)size);
    write_serial_string("\n");

    while (current) {
        write_serial_string("Checking region at ");
        serial_write_hex32((uint32_t)(uintptr_t)current->start);
        write_serial_string(" size ");
        serial_write_dec((int)current->size);
        write_serial_string("\n");

        if (addr + size <= current->start) break;
        addr = current->start + current->size;
        prev = current;
        current = current->next;
    }

    write_serial_string("Chosen addr: ");
    serial_write_hex32((uint32_t)addr);
    write_serial_string("\n");

    if (addr + size > VMM_LIMIT) {
        write_serial_string("Allocation exceeds VMM_LIMIT\n");
        return NULL;
    }

    uintptr_t phys = pmm_alloc_page();
    write_serial_string("pmm_alloc_page returned phys addr: ");
    serial_write_hex32((uint32_t)phys);
    write_serial_string("\n");

    if (!phys) {
        write_serial_string("Out of physical memory!\n");
        return NULL;
    }

    void* temp = vmm_temp_map(phys);
    write_serial_string("vmm_temp_map returned: ");
    serial_write_hex32((uint32_t)(uintptr_t)temp);
    write_serial_string("\n");

    if (!temp) {
        write_serial_string("vmm_temp_map failed!\n");
        return NULL;
    }

    // Test write before memset
    volatile uint8_t* test_ptr = (volatile uint8_t*)temp;
    *test_ptr = 0xAB;
    write_serial_string("Test write to temp mapped memory succeeded\n");

    memset(temp, 0, PAGE_SIZE);
    write_serial_string("memset done\n");

    vmm_region_t* region = (vmm_region_t*)temp;
    region->start = addr;
    region->size = size;
    region->flags = flags;
    region->next = current;

    if (prev) {
        prev->next = region;
    } else {
        vmm_head = region;
    }

    //vmm_temp_unmap();
    write_serial_string("vmm_temp_unmap done\n");

    write_serial_string("vmm_alloc returning addr: ");
    serial_write_hex32((uint32_t)addr);
    write_serial_string("\n");

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
            pmm_free_page((void*)current);
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


void test_vmm() {
    write_serial_string("=== VMM TEST START ===\n");



    void* vaddr = vmm_alloc(2 * 0x1000, 0x3); // allocate 2 pages
    if (!vaddr) {
        write_serial_string("vmm_alloc failed!\n");
        return;
    }

    write_serial_string("vmm_alloc success at: 0x");
    serial_write_hex32((uint32_t)vaddr);
    write_serial_string("\n");

    uintptr_t phys = (uintptr_t)pmm_alloc_page();
    if (!phys) {
        write_serial_string("pmm_alloc_page failed!\n");
        return;
    }

    write_serial_string("pmm_alloc_page success at: 0x");
    serial_write_hex32(phys);
    write_serial_string("\n");

    if (vmm_map(vaddr, phys, 0x1000, 0x3) != 0) {
        write_serial_string("vmm_map failed!\n");
        return;
    }

    write_serial_string("Mapped VA -> PA: 0x");
    serial_write_hex32((uint32_t)vaddr);
    write_serial_string(" -> 0x");
    serial_write_hex32(phys);
    write_serial_string("\n");

    *((uint32_t*)vaddr) = 0xCAFEBABE;
    if (*((uint32_t*)vaddr) == 0xCAFEBABE) {
        write_serial_string("Memory R/W test OK\n");
    } else {
        write_serial_string("Memory R/W test FAIL\n");
    }

    vmm_unmap(vaddr, 0x1000);
    vmm_free(vaddr);
    pmm_free_page((void*)phys);

    write_serial_string("Unmapped, freed VMM and PMM.\n");
    write_serial_string("=== VMM TEST END ===\n");
}