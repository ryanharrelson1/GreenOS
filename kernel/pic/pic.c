#include "pic.h"
#include "../io/io.h"  // for outb and inb functions

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT    0x11
#define ICW4_8086    0x01

void pic_remap(void) {
    const uint8_t offset_master = 0x20; // Master PIC vector offset
    const uint8_t offset_slave  = 0x28; // Slave PIC vector offset

    uint8_t mask_master = inb(PIC1_DATA); // save masks
    uint8_t mask_slave  = inb(PIC2_DATA);

    // Start initialization sequence (in cascade mode)
    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);

    // Set vector offsets
    outb(PIC1_DATA, offset_master);
    outb(PIC2_DATA, offset_slave);

    // Tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 0x04);
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 0x02);

    // Set PICs to 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Restore saved masks
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}