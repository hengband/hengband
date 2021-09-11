#pragma once

#include "system/angband.h"

struct monster_race;
struct monster_type;
struct old_race_flags;;
struct player_type;
struct turn_flags;
bool update_riding_monster(player_type *player_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx);
void update_player_type(player_type *player_ptr, turn_flags *turn_flags_ptr, monster_race *r_ptr);
void update_monster_race_flags(player_type *player_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr);
void update_player_window(player_type *player_ptr, old_race_flags *old_race_flags_ptr);
void update_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool full);
void update_monsters(player_type *player_ptr, bool full);
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what);
