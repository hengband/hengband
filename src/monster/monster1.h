#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

typedef bool (*monsterrace_hook_type)(MONRACE_IDX r_idx);

void roff_top(MONRACE_IDX r_idx);
void screen_roff(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode);
void display_roff(player_type *player_ptr);
void output_monster_spoiler(player_type *player_ptr, MONRACE_IDX r_idx, void (*roff_func)(TERM_COLOR attr, concptr str));
concptr extract_note_dies(MONRACE_IDX r_idx);
void monster_death(player_type *player_ptr, MONSTER_IDX m_idx, bool drop_item);
monsterrace_hook_type get_monster_hook(player_type *player_ptr);
monsterrace_hook_type get_monster_hook2(player_type *player_ptr, POSITION y, POSITION x);
void set_friendly(monster_type *m_ptr);
void set_pet(player_type *player_ptr, monster_type *m_ptr);
void set_hostile(player_type *player_ptr, monster_type *m_ptr);
void anger_monster(player_type *player_ptr, monster_type *m_ptr);

/*
 * Bit flags for the *_can_enter() and monster_can_cross_terrain()
 */
#define CEM_RIDING 0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
bool monster_can_cross_terrain(player_type *player_ptr, FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode);
bool monster_can_enter(player_type *player_ptr, POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode);
bool are_enemies(player_type *player_ptr, monster_type *m_ptr1, monster_type *m_ptr2);
bool monster_has_hostile_align(player_type *player_ptr, monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr);
void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char *msg);
concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode);
bool is_original_ap_and_seen(player_type *player_ptr, monster_type *m_ptr);
