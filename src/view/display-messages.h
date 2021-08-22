#pragma once

#include "system/angband.h"

/*
 * OPTION: Maximum number of messages to remember (see "io.c")
 * Default: assume maximal memorization of 2048 total messages
 */
#define MESSAGE_MAX 81920

extern bool msg_flag;
extern COMMAND_CODE now_message;

int32_t message_num(void);
concptr message_str(int age);
void message_add(concptr msg);
void msg_erase(void);
void msg_print(concptr msg);
void msg_format(concptr fmt, ...);
