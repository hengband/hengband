
/*!
* vaultに配置可能なモンスターの条件を指定するマクロ / Monster validation macro
*
* Line 1 -- forbid town monsters
* Line 2 -- forbid uniques
* Line 3 -- forbid aquatic monsters
*/
#define vault_monster_okay(I) \
	(mon_hook_dungeon(I) && \
	 !(r_info[I].flags1 & RF1_UNIQUE) && \
	 !(r_info[I].flags7 & RF7_UNIQUE2) && \
	 !(r_info[I].flagsr & RFR_RES_ALL) && \
	 !(r_info[I].flags7 & RF7_AQUATIC))

extern int vault_aux_race;
extern char vault_aux_char;
extern BIT_FLAGS vault_aux_dragon_mask4;

extern bool mon_hook_quest(MONRACE_IDX r_idx);
extern bool mon_hook_dungeon(MONRACE_IDX r_idx);
extern bool mon_hook_ocean(MONRACE_IDX r_idx);
extern bool mon_hook_shore(MONRACE_IDX r_idx);
extern bool mon_hook_waste(MONRACE_IDX r_idx);
extern bool mon_hook_town(MONRACE_IDX r_idx);
extern bool mon_hook_wood(MONRACE_IDX r_idx);
extern bool mon_hook_volcano(MONRACE_IDX r_idx);
extern bool mon_hook_wood(MONRACE_IDX r_idx);
extern bool mon_hook_volcano(MONRACE_IDX r_idx);
extern bool mon_hook_mountain(MONRACE_IDX r_idx);
extern bool mon_hook_grass(MONRACE_IDX r_idx);
extern bool mon_hook_deep_water(MONRACE_IDX r_idx);
extern bool mon_hook_shallow_water(MONRACE_IDX r_idx);
extern bool mon_hook_lava(MONRACE_IDX r_idx);
extern bool mon_hook_floor(MONRACE_IDX r_idx);

extern void vault_prep_clone(player_type *player_ptr);
extern void vault_prep_dragon(player_type *player_ptr);
extern void vault_prep_symbol(player_type *player_ptr);

extern bool vault_aux_lite(MONRACE_IDX r_idx);
extern bool vault_aux_shards(MONRACE_IDX r_idx);
extern bool vault_aux_simple(MONRACE_IDX r_idx);
extern bool vault_aux_jelly(MONRACE_IDX r_idx);
extern bool vault_aux_animal(MONRACE_IDX r_idx);
extern bool vault_aux_undead(MONRACE_IDX r_idx);
extern bool vault_aux_chapel_g(MONRACE_IDX r_idx);
extern bool vault_aux_kennel(MONRACE_IDX r_idx);
extern bool vault_aux_mimic(MONRACE_IDX r_idx);
extern bool vault_aux_clone(MONRACE_IDX r_idx);
extern bool vault_aux_symbol_e(MONRACE_IDX r_idx);
extern bool vault_aux_symbol_g(MONRACE_IDX r_idx);
extern bool vault_aux_orc(MONRACE_IDX r_idx);
extern bool vault_aux_troll(MONRACE_IDX r_idx);
extern bool vault_aux_giant(MONRACE_IDX r_idx);
extern bool vault_aux_dragon(MONRACE_IDX r_idx);
extern bool vault_aux_demon(MONRACE_IDX r_idx);
extern bool vault_aux_cthulhu(MONRACE_IDX r_idx);
extern bool vault_aux_dark_elf(MONRACE_IDX r_idx);

extern bool monster_living(MONRACE_IDX r_idx);
extern bool no_questor_or_bounty_uniques(MONRACE_IDX r_idx);
extern bool monster_hook_human(MONRACE_IDX r_idx);
extern bool get_nightmare(MONRACE_IDX r_idx);
extern bool monster_is_fishing_target(MONRACE_IDX r_idx);
extern bool monster_can_entry_arena(MONRACE_IDX r_idx);
extern bool item_monster_okay(MONRACE_IDX r_idx);



