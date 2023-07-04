#pragma once

#include "system/angband.h"
#include <optional>
#include <string_view>
#include <type_traits>

/*
 * Bit flags for control of get_check_strict()
 */
#define CHECK_OKAY_CANCEL 0x01
#define CHECK_NO_ESCAPE 0x02
#define CHECK_NO_HISTORY 0x04
#define CHECK_DEFAULT_Y 0x08

class PlayerType;
bool askfor(char *buf, int len, bool numpad_cursor = true);
std::optional<std::string> input_string(std::string_view prompt, int len, std::string_view initial_value = "");
bool get_check(std::string_view prompt);
bool get_check_strict(PlayerType *player_ptr, std::string_view prompt, BIT_FLAGS mode);
std::optional<char> input_command(std::string_view prompt, bool z_escape = false);
QUANTITY get_quantity(std::optional<std::string_view> prompt_opt, QUANTITY max);
void pause_line(int row);
std::optional<int> input_integer(std::string_view prompt, int min, int max, int initial_value = 0);

template <typename T>
std::optional<T> input_numerics(std::string_view prompt, int min, int max, T initial_value = static_cast<T>(0))
    requires std::is_integral_v<T> || std::is_enum_v<T>
{
    auto result = input_integer(prompt, min, max, static_cast<int>(initial_value));
    if (!result.has_value()) {
        return std::nullopt;
    }

    return std::make_optional(static_cast<T>(result.value()));
}
