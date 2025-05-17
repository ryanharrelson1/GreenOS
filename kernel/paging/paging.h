#pragma once

#include "../stdint.h"
#include <stddef.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 0x1000

typedef struct {
    uint32_t* page_directory;
} paging_context_t;

// Initialize paging and enable paging hardware
void paging_init(void);

// Map a virtual address to a new physical frame with given flags
void* paging_map(uintptr_t virt_addr, uint32_t flags);

// Switch to a new page directory
void paging_switch_dir(uint32_t* new_dir);

// Get current page directory
uint32_t* paging_current_dir(void);

// Identity map a region (virtual == physical)
void paging_ident_map(uintptr_t addr, size_t size, uint32_t flags);

// Map the kernel region in current directory
void paging_map_kernel(uint32_t kernel_start, uint32_t kernel_end);

// Create a new user page directory, copying kernel mappings
uint32_t* paging_create_user_directory(void);

// Map a virtual address for a user directory with flags
void* paging_map_user(uint32_t* dir, uintptr_t virt_addr, uint32_t flags);

// Unmap a virtual address and free physical frame
void paging_unmap(uint32_t* dir, uint32_t virt_addr);