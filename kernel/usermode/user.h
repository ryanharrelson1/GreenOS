#ifndef USER_H
#define USER_H

#include "../stdint.h"


void usermode_init(void);

void enter_user_mode(uintptr_t entry_point, uintptr_t user_stack);

#endif