﻿#pragma once

#include "system/angband.h"
#include <concepts>
#include <string_view>

/*
 * OPTION: Maximum number of messages to remember (see "io.c")
 * Default: assume maximal memorization of 2048 total messages
 */
#define MESSAGE_MAX 81920

extern bool msg_flag;
extern COMMAND_CODE now_message;

int32_t message_num(void);
concptr message_str(int age);
void message_add(std::string_view msg);
void msg_erase(void);
void msg_print(std::string_view msg);
void msg_print(std::nullptr_t);
void msg_format(std::string_view fmt, ...);
void msg_format(const char *fmt, ...) __attribute__((angband::format(printf, 1, 2)));
