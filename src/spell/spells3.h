#pragma once

#include "object/tval-types.h"
#include "spell/spells-util.h"
#include "system/angband.h"

bool eat_magic(player_type *caster_ptr, int power);
bool apply_disenchant(player_type* target_ptr, BIT_FLAGS mode);
void fetch(player_type* caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los);
void reserve_alter_reality(player_type* caster_ptr);
void identify_pack(player_type* target_ptr);
bool alchemy(player_type* caster_ptr);
bool artifact_scroll(player_type* caster_ptr);
bool ident_spell(player_type* caster_ptr, bool only_equip, tval_type item_tester_tval);
bool mundane_spell(player_type* ownner_ptr, bool only_equip);
bool identify_item(player_type* owner_ptr, object_type* o_ptr);
bool identify_fully(player_type* caster_ptr, bool only_equip, tval_type item_tester_tval);
bool recharge(player_type* caster_ptr, int power);
void display_spell_list(player_type* caster_ptr);
EXP experience_of_spell(player_type* caster_ptr, SPELL_IDX spell, REALM_IDX use_realm);
MANA_POINT mod_need_mana(player_type* caster_ptr, MANA_POINT need_mana, SPELL_IDX spell, REALM_IDX realm);
PERCENTAGE mod_spell_chance_1(player_type* caster_ptr, PERCENTAGE chance);
PERCENTAGE mod_spell_chance_2(player_type* caster_ptr, PERCENTAGE chance);
PERCENTAGE spell_chance(player_type* caster_ptr, SPELL_IDX spell, REALM_IDX realm);
void print_spells(player_type* caster_ptr, SPELL_IDX target_spell, SPELL_IDX* spells, int num, TERM_LEN y, TERM_LEN x, REALM_IDX realm);
bool polymorph_monster(player_type* caster_ptr, POSITION y, POSITION x);
void massacre(player_type* caster_ptr);
bool eat_rock(player_type* caster_ptr);
bool shock_power(player_type* caster_ptr);
bool fetch_monster(player_type* caster_ptr);
bool booze(player_type* creature_ptr);
bool detonation(player_type* creature_ptr);
void blood_curse_to_enemy(player_type* caster_ptr, MONSTER_IDX m_idx);
bool fire_crimson(player_type* shooter_ptr);
