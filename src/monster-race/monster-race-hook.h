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

bool vault_aux_shards(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_jelly(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_animal(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_undead(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_chapel_g(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_kennel(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_mimic(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_clone(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_symbol_e(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_symbol_g(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_orc(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_troll(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_giant(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_dragon(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_demon(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_cthulhu(PlayerType *player_ptr, MonraceId r_idx);
bool vault_aux_dark_elf(PlayerType *player_ptr, MonraceId r_idx);

bool monster_hook_human(PlayerType *player_ptr, MonraceId r_idx);
bool get_nightmare(PlayerType *player_ptr, MonraceId r_idx);
bool monster_is_fishing_target(PlayerType *player_ptr, MonraceId r_idx);
bool monster_can_entry_arena(PlayerType *player_ptr, MonraceId r_idx);
bool item_monster_okay(PlayerType *player_ptr, MonraceId r_idx);
