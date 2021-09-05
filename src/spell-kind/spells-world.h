#pragma once

#include "system/angband.h"

struct player_type;
void teleport_level(player_type *creature_ptr, MONSTER_IDX m_idx);
bool teleport_level_other(player_type *caster_ptr);
bool tele_town(player_type *caster_ptr);
void reserve_alter_reality(player_type *caster_ptr, TIME_EFFECT turns);
bool is_teleport_level_ineffective(player_type *caster_ptr, MONSTER_IDX idx);
bool recall_player(player_type *creature_ptr, TIME_EFFECT turns);
bool free_level_recall(player_type *creature_ptr);
bool reset_recall(player_type *caster_ptr);
