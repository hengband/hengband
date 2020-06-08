#pragma once

#include "system/angband.h"

extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;

void set_target(monster_type *m_ptr, POSITION y, POSITION x);
void reset_target(monster_type *m_ptr);
monster_race *real_r_ptr(monster_type *m_ptr);
MONRACE_IDX real_r_idx(monster_type *m_ptr);
void delete_monster_idx(player_type *player_ptr, MONSTER_IDX i);
void compact_monsters(player_type *player_ptr, int size);
void wipe_monsters_list(player_type *player_ptr);
MONSTER_IDX m_pop(player_type *player_ptr);
errr get_mon_num_prep(player_type *player_ptr, monsterrace_hook_type monster_hook, monsterrace_hook_type monster_hook2);

#define GMN_ARENA 0x00000001 //!< 賭け闘技場向け生成
MONRACE_IDX get_mon_num(player_type *player_ptr, DEPTH level, BIT_FLAGS option);
int lore_do_probe(player_type *player_ptr, MONRACE_IDX r_idx);
void lore_treasure(player_type *player_ptr, MONSTER_IDX m_idx, ITEM_NUMBER num_item, ITEM_NUMBER num_gold);
void update_monster(player_type *subject_ptr, MONSTER_IDX m_idx, bool full);
void update_monsters(player_type *player_ptr, bool full);
bool multiply_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode);
bool summon_specific(player_type *player_ptr, MONSTER_IDX who, POSITION y1, POSITION x1, DEPTH lev, int type, BIT_FLAGS mode);
bool summon_named_creature(player_type *player_ptr, MONSTER_IDX who, POSITION oy, POSITION ox, MONRACE_IDX r_idx, BIT_FLAGS mode);
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what);
void choose_new_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx);
SPEED get_mspeed(player_type *player_ptr, monster_race *r_ptr);
void monster_drop_carried_objects(player_type *player_ptr, monster_type *m_ptr);

#define is_friendly(A) (bool)(((A)->smart & SM_FRIENDLY) ? TRUE : FALSE)

#define is_pet(A) (bool)(((A)->smart & SM_PET) ? TRUE : FALSE)

#define is_hostile(A) (bool)((is_friendly(A) || is_pet(A)) ? FALSE : TRUE)

/* Hack -- Determine monster race appearance index is same as race index */
#define is_original_ap(A) (bool)(((A)->ap_r_idx == (A)->r_idx) ? TRUE : FALSE)

int get_monster_crowd_number(player_type *player_ptr, MONSTER_IDX m_idx);
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam);
bool place_monster_aux(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
bool place_monster(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x);
bool alloc_guardian(player_type *player_ptr, bool def_val);
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode);
void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode);
void monster_name(player_type *player_ptr, MONSTER_IDX m_idx, char *m_name);
