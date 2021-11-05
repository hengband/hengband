#pragma once

#include "system/angband.h"

class PlayerType;
MANA_POINT mod_need_mana(PlayerType *player_ptr, MANA_POINT need_mana, SPELL_IDX spell, int16_t realm);
PERCENTAGE mod_spell_chance_1(PlayerType *player_ptr, PERCENTAGE chance);
PERCENTAGE mod_spell_chance_2(PlayerType *player_ptr, PERCENTAGE chance);
PERCENTAGE spell_chance(PlayerType *player_ptr, SPELL_IDX spell, int16_t realm);
void print_spells(PlayerType *player_ptr, SPELL_IDX target_spell, SPELL_IDX *spells, int num, TERM_LEN y, TERM_LEN x, int16_t realm);
