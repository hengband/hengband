#pragma once

#include "system/angband.h"

void reserve_alter_reality(player_type* caster_ptr);
bool artifact_scroll(player_type* caster_ptr);
bool mundane_spell(player_type* ownner_ptr, bool only_equip);
bool recharge(player_type* caster_ptr, int power);
void massacre(player_type* caster_ptr);
bool shock_power(player_type* caster_ptr);
void blood_curse_to_enemy(player_type* caster_ptr, MONSTER_IDX m_idx);
bool fire_crimson(player_type* shooter_ptr);
