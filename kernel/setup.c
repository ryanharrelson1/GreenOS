

__attribute__((section(".text.stub")))
void setup(void){



     volatile char* vga = (volatile char*) 0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i * 2] = 'b';
        vga[i * 2 + 1] = 0x1F; // White on blue
    }



}