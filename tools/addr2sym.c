#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LINE 256

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s kernel.map 0xADDRESS\n", argv[0]);
        return 1;
    }

    const char* map_file = argv[1];
    uintptr_t target_addr = strtoul(argv[2], NULL, 0);

    FILE* file = fopen(map_file, "r");
    if (!file) {
        perror("Failed to open map file");
        return 1;
    }

    char line[MAX_LINE];
    char closest_symbol[128] = "unknown";
    uintptr_t closest_addr = 0;

    while (fgets(line, sizeof(line), file)) {
        uintptr_t addr;
        char symbol[128];

        if (sscanf(line, " %lx %127s", &addr, symbol) == 2) {
            if (addr <= target_addr && addr > closest_addr) {
                closest_addr = addr;
                strcpy(closest_symbol, symbol);
            }
        }
    }

    fclose(file);

    printf("0x%08lx -> %s + 0x%lx\n", target_addr, closest_symbol, target_addr - closest_addr);
    return 0;
}
