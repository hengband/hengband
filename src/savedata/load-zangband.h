#pragma once

#include "system/angband.h"

void load_zangband_options(void);
void set_zangband_realm(player_type *creature_ptr);
void set_zangband_skill(player_type *creature_ptr);
void set_zangband_spells(player_type *creature_ptr);
void set_zangband_race(player_type *creature_ptr);
void set_zangband_bounty_uniques(player_type *creature_ptr);
void set_zangband_timed_effects(player_type *creature_ptr);
void set_zangband_mimic(player_type *creature_ptr);
void set_zangband_holy_aura(player_type *creature_ptr);
void set_zangband_reflection(player_type *creature_ptr);
void rd_zangband_dungeon(void);
void set_zangband_game_turns(player_type *creature_ptr);
