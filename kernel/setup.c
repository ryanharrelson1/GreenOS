#include "consol/serial.h"

void setup(void) __attribute__((section(".early_init")));

void setup(void) {
    init_serial();
    write_serial_string("hello world from setup\n");
}