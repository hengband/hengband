#pragma once

typedef struct
{
	grid_type *g_ptr;
	monster_type *m_ptr;
	monster_type *m_caster_ptr;
	monster_race *r_ptr;
	char killer[80];
	bool seen;
	bool seen_msg;
	bool slept;
	bool obvious;
	bool known;
	bool skipped;
	bool get_angry;
	bool do_poly;
	int do_dist;
	int do_conf;
	int do_stun;
	int do_sleep;
	int do_fear;
	int do_time;
	bool heal_leper;
	GAME_TEXT m_name[MAX_NLEN];
	char m_poss[10];
	PARAMETER_VALUE photo;
	concptr note;
	concptr note_dies;
	DEPTH caster_lev;

	MONSTER_IDX who;
	POSITION r;
	POSITION y;
	POSITION x;
	HIT_POINT dam;
	EFFECT_ID effect_type;
	BIT_FLAGS flag;
	bool see_s_msg;
} effect_monster;

void initialize_effect_monster(player_type *caster_ptr, effect_monster *effect_monster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, bool see_s_msg);
