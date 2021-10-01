#pragma once

#include "system/angband.h"
#include <vector>

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

extern const std::vector<option_type> option_info;
extern const std::vector<option_type> cheat_info;
extern const std::vector<option_type> autosave_info;
