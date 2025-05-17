#ifndef SERIAL_H
#define SERIAL_H

#include "../stdint.h"


void init_serial();

void write_serial(char c);

void write_serial_string(const char* str);
void serial_write_dec(int num);
void serial_write_hex32(uint32_t num);
void serial_write_hex64(uint64_t num);


#endif
