#pragma once

#include "system/angband.h"

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

extern int vault_aux_race;
extern char vault_aux_char;
extern EnumClassFlagGroup<RF_ABILITY> vault_aux_dragon_mask4;

struct player_type;
bool mon_hook_quest(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_dungeon(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_ocean(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_shore(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_waste(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_town(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_wood(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_volcano(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_wood(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_volcano(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_mountain(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_grass(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_deep_water(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_shallow_water(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_lava(player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_floor(player_type *player_ptr, MONRACE_IDX r_idx);

void vault_prep_clone(player_type *player_ptr);
void vault_prep_dragon(player_type *player_ptr);
void vault_prep_symbol(player_type *player_ptr);

bool vault_aux_lite(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_shards(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_simple(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_jelly(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_animal(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_undead(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_chapel_g(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_kennel(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_mimic(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_clone(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_symbol_e(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_symbol_g(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_orc(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_troll(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_giant(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_dragon(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_demon(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_cthulhu(player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_dark_elf(player_type *player_ptr, MONRACE_IDX r_idx);

bool vault_monster_okay(player_type *player_ptr, MONRACE_IDX r_idx);

bool monster_living(MONRACE_IDX r_idx);
bool no_questor_or_bounty_uniques(MONRACE_IDX r_idx);
bool monster_hook_human(player_type *player_ptr, MONRACE_IDX r_idx);
bool get_nightmare(player_type *player_ptr, MONRACE_IDX r_idx);
bool monster_is_fishing_target(player_type *player_ptr, MONRACE_IDX r_idx);
bool monster_can_entry_arena(player_type *player_ptr, MONRACE_IDX r_idx);
bool item_monster_okay(player_type *player_ptr, MONRACE_IDX r_idx);
