#pragma once

#include "system/angband.h"

extern u32b message__next;
extern u32b message__last;
extern u32b message__head;
extern u32b message__tail;
extern u32b *message__ptr;
extern char *message__buf;

extern bool msg_flag;

s32b message_num(void);
concptr message_str(int age);
void message_add(concptr msg);
void msg_erase(void);
void msg_print(concptr msg);
void msg_print_wizard(int cheat_type, concptr msg);

#ifndef SWIG
void msg_format(concptr fmt, ...);
void msg_format_wizard(int cheat_type, concptr fmt, ...);
#endif
