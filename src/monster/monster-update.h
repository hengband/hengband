#pragma once

#include "system/angband.h"

typedef struct monster_race monster_race;
typedef struct monster_type monster_type;
typedef struct old_race_flags old_race_flags;
typedef struct player_type player_type;
typedef struct turn_flags turn_flags;
bool update_riding_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx);
void update_player_type(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_race *r_ptr);
void update_monster_race_flags(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr);
void update_player_window(player_type *target_ptr, old_race_flags *old_race_flags_ptr);
void update_monster(player_type *subject_ptr, MONSTER_IDX m_idx, bool full);
void update_monsters(player_type *player_ptr, bool full);
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what);
