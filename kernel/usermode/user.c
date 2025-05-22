#include "../gdt/gdt.h"
#include "user.h"
#include "../paging/paging.h"
#include "../memset.h"
#include "../consol/serial.h"


#define USER_CODE_VIRT  0x00400000
#define USER_STACK_TOP  0xBFFFF000

extern uint8_t _binary_user_program_start[];
extern uint8_t _binary_user_program_end[];



void usermode_init(void){

  write_serial_string("setting up usermode");

    uint32_t* user_directory = paging_create_user_directory();

   
    paging_map_user(user_directory, USER_CODE_VIRT, PAGE_RX );

    for (uintptr_t i = 0; i < 2 * PAGE_SIZE; i += PAGE_SIZE) {
        paging_map_user(user_directory, USER_STACK_TOP - i, PAGE_RW); 
    }

    size_t prog_size = _binary_user_program_end - _binary_user_program_start;
    memcpy((void*)USER_CODE_VIRT, _binary_user_program_start, prog_size);



      paging_switch_dir(user_directory);

       write_serial_string("jumping to usermode ring 3");

        enter_user_mode(USER_CODE_VIRT, USER_STACK_TOP);

}



