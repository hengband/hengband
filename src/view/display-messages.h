#pragma once

#include "system/angband.h"
#include <concepts>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/*!
 * 定義読み込み中の診断を、画面表示や入力待ちをせずに収集する。
 * 捕捉中の msg_print() は履歴にも残らないため、捕捉したメッセージを使わずに破棄すると失われる。
 */
class ScopedMessageCapture {
public:
    ScopedMessageCapture();
    ~ScopedMessageCapture();
    ScopedMessageCapture(const ScopedMessageCapture &) = delete;
    ScopedMessageCapture(ScopedMessageCapture &&) = delete;
    ScopedMessageCapture &operator=(const ScopedMessageCapture &) = delete;
    ScopedMessageCapture &operator=(ScopedMessageCapture &&) = delete;

    const std::vector<std::string> &messages() const;

private:
    std::vector<std::string> captured_messages;
    std::vector<std::string> *previous_capture;
};

constexpr auto MESSAGE_MAX = 81920; //!< 覚えておくメッセージの最大数

extern bool msg_flag;
extern COMMAND_CODE now_message;

int32_t message_num();
uint64_t message_sequence();
std::shared_ptr<const std::string> message_str(int age);
void message_add(std::string_view msg);
void msg_erase();
bool is_waiting_for_more();
void redisplay_more_prompt();
void msg_print(std::string_view msg);
void wr_message_history();
void rd_message_history();

template <typename... Args>
void msg_print(fmt::format_string<Args...> fmt, Args &&...args)
{
    return msg_print(fmt::format(fmt, std::forward<Args>(args)...));
}
