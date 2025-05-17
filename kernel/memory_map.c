#include "memory_map.h"
#include "multiboot2.h"
#include <stddef.h> // for size_t
#include "consol/serial.h"
#include "pmm/pmm.h"

#define MAX_REGIONS 32

struct mem_region mem_regions[MAX_REGIONS]; // for use by pmm
size_t region_count = 0;


void parse_memory_map(uintptr_t mb_info_addr){

    struct multiboot_tag *tag = (struct multiboot_tag *)(mb_info_addr + 8);


    while (tag->type != MULTIBOOT_TAG_TYPE_END)
    {
        if(tag->type == MULTIBOOT_TAG_TYPE_MMAP){
            struct multiboot_tag_mmap *mmap_tag = (struct multiboot_tag_mmap *)tag;

             uintptr_t entry_addr = (uintptr_t)mmap_tag + sizeof(struct multiboot_tag_mmap);

             size_t entries_length = mmap_tag->size - sizeof(struct multiboot_tag_mmap);
          
             for (uintptr_t offset = 0; offset < entries_length; offset += mmap_tag->entry_size) {
                struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)(entry_addr + offset);


                 if(entry->type == 1 && region_count < MAX_REGIONS) {
                    mem_regions[region_count].base_addr = entry->addr;
                    mem_regions[region_count].length = entry->len;
                    mem_regions[region_count].type = entry->type;
                    region_count++;

                }

            }

        }
        tag = (struct multiboot_tag *)((uintptr_t)tag + ((tag->size + 7) & ~7));
    }
    pmm_init(mem_regions, region_count);
    
}
