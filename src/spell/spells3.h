#pragma once

#include "system/angband.h"

bool apply_disenchant(player_type* target_ptr, BIT_FLAGS mode);
void fetch(player_type* caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los);
void reserve_alter_reality(player_type* caster_ptr);
bool alchemy(player_type* caster_ptr);
bool artifact_scroll(player_type* caster_ptr);
bool mundane_spell(player_type* ownner_ptr, bool only_equip);
bool recharge(player_type* caster_ptr, int power);
void display_spell_list(player_type* caster_ptr);
bool polymorph_monster(player_type* caster_ptr, POSITION y, POSITION x);
void massacre(player_type* caster_ptr);
bool eat_rock(player_type* caster_ptr);
bool shock_power(player_type* caster_ptr);
bool fetch_monster(player_type* caster_ptr);
bool booze(player_type* creature_ptr);
bool detonation(player_type* creature_ptr);
void blood_curse_to_enemy(player_type* caster_ptr, MONSTER_IDX m_idx);
bool fire_crimson(player_type* shooter_ptr);
