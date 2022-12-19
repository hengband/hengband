#pragma once

#include "system/angband.h"
#include <optional>
#include <string_view>

/*
 * Bit flags for control of get_check_strict()
 */
#define CHECK_OKAY_CANCEL 0x01
#define CHECK_NO_ESCAPE 0x02
#define CHECK_NO_HISTORY 0x04
#define CHECK_DEFAULT_Y 0x08

class PlayerType;
bool askfor(char *buf, int len, bool numpad_cursor = true);
bool get_string(std::string_view prompt, char *buf, int len);
bool get_check(std::string_view prompt);
bool get_check_strict(PlayerType *player_ptr, std::string_view prompt, BIT_FLAGS mode);
bool get_com(std::string_view prompt, char *command, bool z_escape);
QUANTITY get_quantity(std::optional<std::string_view> prompt_opt, QUANTITY max);
void pause_line(int row);
bool get_value(std::string_view prompt, int min, int max, int *value);
