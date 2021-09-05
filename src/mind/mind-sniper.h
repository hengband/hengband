#pragma once

#include "system/angband.h"

struct monster_type;
struct player_type;
void reset_concentration(player_type *creature_ptr, bool msg);
void display_snipe_list(player_type *sniper_ptr);
MULTIPLY calc_snipe_damage_with_slay(player_type *sniper_ptr, MULTIPLY mult, monster_type *m_ptr, SPELL_IDX snipe_type);
void do_cmd_snipe(player_type *sniper_ptr);
void do_cmd_snipe_browse(player_type *sniper_ptr);
int boost_concentration_damage(player_type *creature_ptr, int tdam);
