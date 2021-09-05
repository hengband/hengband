#pragma once

#include "system/angband.h"
#include <array>

/*
 * Available "options"
 *	- Address of actual option variable (or nullptr)
 *	- Normal Value (TRUE or FALSE)
 *	- Option Page Number (or zero)
 *	- Savefile Set (or zero)
 *	- Savefile Bit in that set
 *	- Textual name (or nullptr)
 *	- Textual description
 */
struct option_type {
    bool *o_var{};
    byte o_norm{};
    byte o_page{};
    byte o_set{};
    byte o_bit{};
    concptr o_text{};
    concptr o_desc{};
};

#define MAX_OPTION_INFO 126
#define MAX_CHEAT_OPTIONS 11
#define MAX_AUTOSAVE_INFO 2

extern const std::array<const option_type, MAX_OPTION_INFO> option_info;
extern const option_type cheat_info[MAX_CHEAT_OPTIONS];
extern const option_type autosave_info[MAX_AUTOSAVE_INFO];
