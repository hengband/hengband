#pragma once

#include "system/angband.h"

/*
 * OPTION: Maximum number of messages to remember (see "io.c")
 * Default: assume maximal memorization of 2048 total messages
 */
#define MESSAGE_MAX 81920

/*
 * OPTION: Maximum space for the message text buffer (see "io.c")
 * Default: assume that each of the 2048 messages is repeated an
 * average of three times, and has an average length of 48
 */
#define MESSAGE_BUF 655360

extern u32b message__next;
extern u32b message__last;
extern u32b message__head;
extern u32b message__tail;
extern u32b *message__ptr;
extern char *message__buf;

extern bool msg_flag;
extern COMMAND_CODE now_message;

s32b message_num(void);
concptr message_str(int age);
void message_add(concptr msg);
void msg_erase(void);
void msg_print(concptr msg);
void msg_format(concptr fmt, ...);
