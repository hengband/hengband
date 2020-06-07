#pragma once

#include "monster/monster-timed-effect-types.h"
#include "monster-race/monster-race.h"

typedef bool(*monsterrace_hook_type)(MONRACE_IDX r_idx);

extern MONSTER_IDX hack_m_idx;
extern MONSTER_IDX hack_m_idx_ii;

/*
 * Monster information, for a specific monster.
 * Note: fy, fx constrain dungeon size to 256x256
 * The "hold_o_idx" field points to the first object of a stack
 * of objects (if any) being carried by the monster (see above).
 */
typedef struct floor_type floor_type;
typedef struct monster_type {
	MONRACE_IDX r_idx;		/* Monster race index 0 = dead. */
	MONRACE_IDX ap_r_idx;	/* Monster race appearance index */
	floor_type *current_floor_ptr;

	/* Sub-alignment flags for neutral monsters */
	#define SUB_ALIGN_NEUTRAL 0x0000
	#define SUB_ALIGN_EVIL    0x0001
	#define SUB_ALIGN_GOOD    0x0002
	BIT_FLAGS8 sub_align;		/* Sub-alignment for a neutral monster */

	POSITION fy;		/* Y location on map */
	POSITION fx;		/* X location on map */
	HIT_POINT hp;		/* Current Hit points */
	HIT_POINT maxhp;		/* Max Hit points */
	HIT_POINT max_maxhp;		/* Max Max Hit points */
	HIT_POINT dealt_damage;		/* Sum of damages dealt by player */
	TIME_EFFECT mtimed[MAX_MTIMED];	/* Timed status counter */
	SPEED mspeed;	        /* Monster "speed" */
	ACTION_ENERGY energy_need;	/* Monster "energy" */
	POSITION cdis;		/* Current dis from player */
	BIT_FLAGS8 mflag;	/* Extra monster flags */
	BIT_FLAGS8 mflag2;	/* Extra monster flags */
	bool ml;		/* Monster is "visible" */
	OBJECT_IDX hold_o_idx;	/* Object being held (if any) */
	POSITION target_y;		/* Can attack !los player */
	POSITION target_x;		/* Can attack !los player */
	STR_OFFSET nickname;		/* Monster's Nickname */
	EXP exp;

	/* TODO: クローン、ペット、有効化は意義が異なるので別変数に切り離すこと。save/loadのバージョン更新が面倒そうだけど */
	BIT_FLAGS smart; /*!< Field for "smart_learn" - Some bit-flags for the "smart" field */
	MONSTER_IDX parent_m_idx;
} monster_type;

/* monster1.c */
void roff_top(MONRACE_IDX r_idx);
void screen_roff(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode);
void display_roff(player_type *player_ptr);
void output_monster_spoiler(player_type *player_ptr, MONRACE_IDX r_idx, void(*roff_func)(TERM_COLOR attr, concptr str));
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
#define CEM_RIDING              0x0001
#define CEM_P_CAN_ENTER_PATTERN 0x0002
bool monster_can_cross_terrain(player_type *player_ptr, FEAT_IDX feat, monster_race *r_ptr, BIT_FLAGS16 mode);
bool monster_can_enter(player_type *player_ptr, POSITION y, POSITION x, monster_race *r_ptr, BIT_FLAGS16 mode);
bool are_enemies(player_type *player_ptr, monster_type *m_ptr1, monster_type *m_ptr2);
bool monster_has_hostile_align(player_type *player_ptr, monster_type *m_ptr, int pa_good, int pa_evil, monster_race *r_ptr);
void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char* msg);
concptr look_mon_desc(monster_type *m_ptr, BIT_FLAGS mode);
bool is_original_ap_and_seen(player_type *player_ptr, monster_type *m_ptr);

/* monster2.c */
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

#define is_friendly(A) \
	 (bool)(((A)->smart & SM_FRIENDLY) ? TRUE : FALSE)

#define is_pet(A) \
	 (bool)(((A)->smart & SM_PET) ? TRUE : FALSE)

#define is_hostile(A) \
	 (bool)((is_friendly(A) || is_pet(A)) ? FALSE : TRUE)

/* Hack -- Determine monster race appearance index is same as race index */
#define is_original_ap(A) \
	 (bool)(((A)->ap_r_idx == (A)->r_idx) ? TRUE : FALSE)

int get_monster_crowd_number(player_type *player_ptr, MONSTER_IDX m_idx);
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam);
bool place_monster_aux(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode);
bool place_monster(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode);
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x);
bool alloc_guardian(player_type *player_ptr, bool def_val);
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode);
void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode);
void monster_name(player_type *player_ptr, MONSTER_IDX m_idx, char *m_name);
