#pragma once

#include "system/angband.h"

extern int vault_aux_race;
extern char vault_aux_char;
extern BIT_FLAGS vault_aux_dragon_mask4;

bool mon_hook_quest(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_dungeon(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_ocean(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_shore(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_waste(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_town(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_wood(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_volcano(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_wood(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_volcano(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_mountain(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_grass(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_deep_water(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_shallow_water(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_lava(const player_type *player_ptr, MONRACE_IDX r_idx);
bool mon_hook_floor(const player_type *player_ptr, MONRACE_IDX r_idx);

void vault_prep_clone(const player_type *player_ptr);
void vault_prep_dragon(const player_type *player_ptr);
void vault_prep_symbol(const player_type *player_ptr);

bool vault_aux_lite(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_shards(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_simple(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_jelly(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_animal(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_undead(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_chapel_g(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_kennel(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_mimic(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_clone(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_symbol_e(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_symbol_g(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_orc(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_troll(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_giant(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_dragon(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_demon(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_cthulhu(const player_type *player_ptr, MONRACE_IDX r_idx);
bool vault_aux_dark_elf(const player_type *player_ptr, MONRACE_IDX r_idx);

bool vault_monster_okay(const player_type *player_ptr, MONRACE_IDX r_idx);

bool monster_living(MONRACE_IDX r_idx);
bool no_questor_or_bounty_uniques(MONRACE_IDX r_idx);
bool monster_hook_human(const player_type *player_ptr, MONRACE_IDX r_idx);
bool get_nightmare(const player_type *player_ptr, MONRACE_IDX r_idx);
bool monster_is_fishing_target(const player_type *player_ptr, MONRACE_IDX r_idx);
bool monster_can_entry_arena(const player_type *player_ptr, MONRACE_IDX r_idx);
bool item_monster_okay(const player_type *player_ptr, MONRACE_IDX r_idx);
