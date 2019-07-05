#pragma once

/*
 * A structure to hold "rolled" information
 */
typedef struct birther birther;

struct birther
{
	SEX_IDX psex;		/* Sex index */
	RACE_IDX prace;		/* Race index */
	CLASS_IDX pclass;	/* Class index */
	CHARACTER_IDX pseikaku;	/* Seikaku index */
	REALM_IDX realm1;	/* First magic realm */
	REALM_IDX realm2;	/* Second magic realm */

	s16b age;
	s16b ht;
	s16b wt;
	s16b sc;

	PRICE au; /*!< 初期の所持金 */

	BASE_STATUS stat_max[6];	/* Current "maximal" stat values */
	BASE_STATUS stat_max_max[6];	/* Maximal "maximal" stat values */
	HIT_POINT player_hp[PY_MAX_LEVEL];

	PATRON_IDX chaos_patron;

	s16b vir_types[8];

	char history[4][60];

	bool quick_ok;
};

extern birther previous_char;

/* birth.c */
extern void add_history_from_pref_line(concptr t);
extern void player_birth(player_type *creature_ptr);
extern void get_max_stats(player_type *creature_ptr);
extern void get_height_weight(player_type *creature_ptr);
extern void player_outfit(void);
extern void dump_yourself(player_type *creature_ptr, FILE *fff);

