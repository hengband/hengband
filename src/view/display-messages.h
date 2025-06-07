#pragma once

#include "system/angband.h"
#include <concepts>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <string_view>

/*
 * OPTION: Maximum number of messages to remember (see "io.c")
 * Default: assume maximal memorization of 2048 total messages
 */
#define MESSAGE_MAX 81920

extern bool msg_flag;
extern COMMAND_CODE now_message;

int32_t message_num();
std::shared_ptr<const std::string> message_str(int age);
void message_add(std::string_view msg);
void msg_erase();
void msg_print(std::string_view msg);
void msg_format(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void wr_message_history();
void rd_message_history();

template <typename... Args>
void msg_print(fmt::format_string<Args...> fmt, Args &&...args)
{
    return msg_print(fmt::format(fmt, std::forward<Args>(args)...));
}
