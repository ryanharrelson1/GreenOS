#include "../alarm/panic.h"
#include "../stdint.h"
#include "../consol/serial.h"

// Declare handlers to be called from assembly stubs
void isr_divide_by_zero_stub_handler(int int_no, uint32_t error_code);
void isr_double_fault_stub_handler(int int_no, uint32_t error_code);
void isr_gpf_stub_handler(int int_no, uint32_t error_code);
void isr_page_fault_stub_handler(int int_no, uint32_t error_code);
void isr_generic_exception_stub_handler(int int_no, uint32_t error_code);


// Then rename your previous C handlers like this, or make wrapper functions
void isr_divide_by_zero_stub_handler(int int_no, uint32_t error_code) {
    
    serial_write_dec(int_no);
    serial_write_hex32(error_code);
    panic("Exception: Divide By Zero");
}
void isr_double_fault_stub_handler(int int_no, uint32_t error_code) {
    serial_write_dec(int_no);
    serial_write_hex32(error_code);
    panic("Exception: Double Fault");
}
void isr_gpf_stub_handler(int int_no, uint32_t error_code) {
  serial_write_dec(int_no);
  serial_write_hex32(error_code);

    panic("Exception: General Protection Fault");
}


void isr_page_fault_stub_handler(int int_no, uint32_t error_code) {
    uint32_t faulting_address;
    asm volatile ("mov %%cr2, %0" : "=r" (faulting_address));

    serial_write_hex32(faulting_address);
    
    // You might want a better formatted message but keep it simple for now
    panic("Exception: Page Fault at address %x");
}
void isr_generic_exception_stub_handler(int int_no, uint32_t error_code) {

    serial_write_dec(int_no);

    serial_write_hex32(error_code);
    panic("Exception: Unknown Interrupt %d, error code %d");
}