#include "../gdt/gdt.h"
#include "user.h"
#include "../paging/paging.h"


#define USER_CODE_VIRT  0x00400000
#define USER_STACK_TOP  0xBFFFF000



void usermode_init(void){
    uint32_t* user_directory = paging_create_user_directory();

   
    paging_map_user(user_directory, USER_CODE_VIRT, PAGE_RX );

    for (uintptr_t i = 0; i < 2 * PAGE_SIZE; i += PAGE_SIZE) {
        paging_map_user(user_directory, USER_STACK_TOP - i, PAGE_RW); 
    }


      paging_switch_dir(user_directory);

        enter_user_mode(USER_CODE_VIRT, USER_STACK_TOP);

}



