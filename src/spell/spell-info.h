#pragma once

#include "system/angband.h"

struct player_type;
EXP experience_of_spell(player_type *player_ptr, SPELL_IDX spell, int16_t use_realm);
MANA_POINT mod_need_mana(player_type *player_ptr, MANA_POINT need_mana, SPELL_IDX spell, int16_t realm);
PERCENTAGE mod_spell_chance_1(player_type *player_ptr, PERCENTAGE chance);
PERCENTAGE mod_spell_chance_2(player_type *player_ptr, PERCENTAGE chance);
PERCENTAGE spell_chance(player_type *player_ptr, SPELL_IDX spell, int16_t realm);
void print_spells(player_type *player_ptr, SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, int16_t realm);
