#pragma once

#include "system/angband.h"

class PlayerType;
enum class RealmType;
MANA_POINT mod_need_mana(PlayerType *player_ptr, MANA_POINT need_mana, SPELL_IDX spell_id, RealmType realm);
PERCENTAGE mod_spell_chance_1(PlayerType *player_ptr, PERCENTAGE chance);
PERCENTAGE mod_spell_chance_2(PlayerType *player_ptr, PERCENTAGE chance);
PERCENTAGE spell_chance(PlayerType *player_ptr, SPELL_IDX spell_id, RealmType realm);
void print_spells(PlayerType *player_ptr, SPELL_IDX target_spell_id, const SPELL_IDX *spell_ids, int num, TERM_LEN y, TERM_LEN x, RealmType realm);
