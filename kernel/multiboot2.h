#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include "stdint.h"

// Common tag header
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

// Multiboot tag types
#define MULTIBOOT_TAG_TYPE_END       0
#define MULTIBOOT_TAG_TYPE_MMAP      6

// Memory map entry types
#define MULTIBOOT_MEMORY_AVAILABLE   1
#define MULTIBOOT_MEMORY_RESERVED    2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS         4
#define MULTIBOOT_MEMORY_BADRAM      5

// Memory map tag structure
struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    // Followed by one or more entries:
    // struct multiboot_mmap_entry entries[];
};

// Memory map entry structure
struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
};

#endif
