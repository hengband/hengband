/*!
 * @brief monster-processのための構造体群
 * @date 2020/03/07
 * @author Hourier
 */

#pragma once

#include "system/angband.h"
#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

typedef struct turn_flags {
	bool see_m;
	bool aware;
	bool is_riding_mon;
	bool do_turn;
	bool do_move;
	bool do_view;
	bool do_take;
	bool must_alter_to_move;

	bool did_open_door;
	bool did_bash_door;
	bool did_take_item;
	bool did_kill_item;
	bool did_move_body;
	bool did_pass_wall;
	bool did_kill_wall;
} turn_flags;

typedef struct old_race_flags {
	BIT_FLAGS old_r_flags1;
	BIT_FLAGS old_r_flags2;
	BIT_FLAGS old_r_flags3;
	BIT_FLAGS old_r_flagsr;
	EnumClassFlagGroup<MonsterAbilityType> old_r_ability_flags;

	byte old_r_blows0;
	byte old_r_blows1;
	byte old_r_blows2;
	byte old_r_blows3;

	byte old_r_cast_spell;
} old_race_flags;

typedef struct coordinate_candidate {
	POSITION gy;
	POSITION gx;
	POSITION gdis;
} coordinate_candidate;

struct monster_type;
turn_flags *init_turn_flags(MONSTER_IDX riding_idx, MONSTER_IDX m_idx, turn_flags *turn_flags_ptr);
old_race_flags *init_old_race_flags(old_race_flags *old_race_flags_ptr);
coordinate_candidate init_coordinate_candidate(void);

void store_enemy_approch_direction(int *mm, POSITION y, POSITION x);
void store_moves_val(int *mm, int y, int x);
void save_old_race_flags(MONRACE_IDX monster_race_idx, old_race_flags *old_race_flags_ptr);
SPEED decide_monster_speed(monster_type *m_ptr);
