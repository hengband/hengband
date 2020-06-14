#pragma once

#include "system/angband.h"

extern int vault_aux_race;
extern char vault_aux_char;
extern BIT_FLAGS vault_aux_dragon_mask4;

bool mon_hook_quest(MONRACE_IDX r_idx);
bool mon_hook_dungeon(MONRACE_IDX r_idx);
bool mon_hook_ocean(MONRACE_IDX r_idx);
bool mon_hook_shore(MONRACE_IDX r_idx);
bool mon_hook_waste(MONRACE_IDX r_idx);
bool mon_hook_town(MONRACE_IDX r_idx);
bool mon_hook_wood(MONRACE_IDX r_idx);
bool mon_hook_volcano(MONRACE_IDX r_idx);
bool mon_hook_wood(MONRACE_IDX r_idx);
bool mon_hook_volcano(MONRACE_IDX r_idx);
bool mon_hook_mountain(MONRACE_IDX r_idx);
bool mon_hook_grass(MONRACE_IDX r_idx);
bool mon_hook_deep_water(MONRACE_IDX r_idx);
bool mon_hook_shallow_water(MONRACE_IDX r_idx);
bool mon_hook_lava(MONRACE_IDX r_idx);
bool mon_hook_floor(MONRACE_IDX r_idx);

void vault_prep_clone(player_type *player_ptr);
void vault_prep_dragon(player_type *player_ptr);
void vault_prep_symbol(player_type *player_ptr);

bool vault_aux_lite(MONRACE_IDX r_idx);
bool vault_aux_shards(MONRACE_IDX r_idx);
bool vault_aux_simple(MONRACE_IDX r_idx);
bool vault_aux_jelly(MONRACE_IDX r_idx);
bool vault_aux_animal(MONRACE_IDX r_idx);
bool vault_aux_undead(MONRACE_IDX r_idx);
bool vault_aux_chapel_g(MONRACE_IDX r_idx);
bool vault_aux_kennel(MONRACE_IDX r_idx);
bool vault_aux_mimic(MONRACE_IDX r_idx);
bool vault_aux_clone(MONRACE_IDX r_idx);
bool vault_aux_symbol_e(MONRACE_IDX r_idx);
bool vault_aux_symbol_g(MONRACE_IDX r_idx);
bool vault_aux_orc(MONRACE_IDX r_idx);
bool vault_aux_troll(MONRACE_IDX r_idx);
bool vault_aux_giant(MONRACE_IDX r_idx);
bool vault_aux_dragon(MONRACE_IDX r_idx);
bool vault_aux_demon(MONRACE_IDX r_idx);
bool vault_aux_cthulhu(MONRACE_IDX r_idx);
bool vault_aux_dark_elf(MONRACE_IDX r_idx);

bool vault_monster_okay(MONRACE_IDX r_idx);

bool monster_living(MONRACE_IDX r_idx);
bool no_questor_or_bounty_uniques(MONRACE_IDX r_idx);
bool monster_hook_human(MONRACE_IDX r_idx);
bool get_nightmare(MONRACE_IDX r_idx);
bool monster_is_fishing_target(MONRACE_IDX r_idx);
bool monster_can_entry_arena(MONRACE_IDX r_idx);
bool item_monster_okay(MONRACE_IDX r_idx);
