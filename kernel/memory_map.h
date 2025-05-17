#ifndef MEMORY_H
#define MEMORY_H

#include "stdint.h"

struct mem_region {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type; // optionally store the type for debug/logging
};

// Call this to parse the multiboot memory map
void parse_memory_map(uintptr_t mb_info_addr);

#endif