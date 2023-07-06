#pragma once

#include "util/flag-group.h"
#include <optional>
#include <string_view>
#include <type_traits>

enum class UserCheck {
    NONE = 0,
    OKAY_CANCEL = 1,
    NO_ESCAPE = 2,
    NO_HISTORY = 3,
    DEFAULT_Y = 4,
    MAX = 5,
};

class PlayerType;
bool askfor(char *buf, int len, bool numpad_cursor = true);
std::optional<std::string> input_string(std::string_view prompt, int len, std::string_view initial_value = "", bool numpad_cursor = true);
bool input_check(std::string_view prompt);
bool input_check_strict(PlayerType *player_ptr, std::string_view prompt, UserCheck one_mode);
bool input_check_strict(PlayerType *player_ptr, std::string_view prompt, EnumClassFlagGroup<UserCheck> mode);
std::optional<char> input_command(std::string_view prompt, bool z_escape = false);
int input_quantity(int max, std::string_view initial_prompt = "");
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
