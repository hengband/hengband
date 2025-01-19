#pragma once

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

enum class MonraceId : short;

extern MonraceId vault_aux_race;
extern char vault_aux_char;
extern EnumClassFlagGroup<MonsterAbilityType> vault_aux_dragon_mask4;

class PlayerType;
void vault_prep_clone(PlayerType *player_ptr);
void vault_prep_dragon(PlayerType *player_ptr);
void vault_prep_symbol(PlayerType *player_ptr);

bool vault_aux_jelly(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_animal(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_undead(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_chapel_g(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_kennel(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_clone(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_symbol_e(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_symbol_g(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_dragon(PlayerType *player_ptr, MonraceId r_idx);
