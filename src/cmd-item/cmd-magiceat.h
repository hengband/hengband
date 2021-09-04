#pragma once

#define EATER_EXT 36
#define EATER_CHARGE 0x10000L
#define EATER_ROD_CHARGE 0x10L

struct player_type;
bool do_cmd_magic_eater(player_type *creature_ptr, bool only_browse, bool powerful);
