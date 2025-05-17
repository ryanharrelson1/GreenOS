#pragma once

#include "../stdint.h"

void panic(const char* message);


uint32_t get_eip();
void dump_cpu_registers();
void panic_print_backtrace();
void print_stack(uint32_t* esp, int words);