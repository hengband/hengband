#pragma once

#include "system/angband.h"
#include <fmt/format.h>
#include <string_view>
#include <utility>

class PlayerType;
bool is_wizard_message_enabled(int cheat_type);
void msg_print_wizard(PlayerType *player_ptr, int cheat_type, std::string_view msg);

template <typename... Args>
void msg_print_wizard(PlayerType *player_ptr, int cheat_type, fmt::format_string<Args...> fmt, Args &&...args)
{
    // 表示しない種類のメッセージは書式を組み立てない
    if (!is_wizard_message_enabled(cheat_type)) {
        return;
    }

    msg_print_wizard(player_ptr, cheat_type, fmt::format(fmt, std::forward<Args>(args)...));
}
