#pragma once

#include "system/angband.h"

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

enum class MonsterRaceId : int16_t;

extern MonsterRaceId vault_aux_race;
extern char vault_aux_char;
extern EnumClassFlagGroup<MonsterAbilityType> vault_aux_dragon_mask4;

class PlayerType;
bool mon_hook_quest(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_dungeon(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_ocean(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_shore(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_waste(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_town(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_wood(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_volcano(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_wood(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_volcano(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_mountain(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_grass(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_deep_water(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_shallow_water(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_lava(PlayerType *player_ptr, MonsterRaceId r_idx);
bool mon_hook_floor(PlayerType *player_ptr, MonsterRaceId r_idx);

void vault_prep_clone(PlayerType *player_ptr);
void vault_prep_dragon(PlayerType *player_ptr);
void vault_prep_symbol(PlayerType *player_ptr);

bool vault_aux_lite(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_shards(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_simple(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_jelly(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_animal(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_undead(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_chapel_g(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_kennel(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_mimic(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_clone(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_symbol_e(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_symbol_g(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_orc(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_troll(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_giant(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_dragon(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_demon(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_cthulhu(PlayerType *player_ptr, MonsterRaceId r_idx);
bool vault_aux_dark_elf(PlayerType *player_ptr, MonsterRaceId r_idx);

bool vault_monster_okay(PlayerType *player_ptr, MonsterRaceId r_idx);

bool monster_living(MonsterRaceId r_idx);
bool no_questor_or_bounty_uniques(MonsterRaceId r_idx);
bool monster_hook_human(PlayerType *player_ptr, MonsterRaceId r_idx);
bool get_nightmare(PlayerType *player_ptr, MonsterRaceId r_idx);
bool monster_is_fishing_target(PlayerType *player_ptr, MonsterRaceId r_idx);
bool monster_can_entry_arena(PlayerType *player_ptr, MonsterRaceId r_idx);
bool item_monster_okay(PlayerType *player_ptr, MonsterRaceId r_idx);
