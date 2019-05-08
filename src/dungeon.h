#pragma once
#include "feature.h"

/* A structure for the != dungeon types */
typedef struct dungeon_type dungeon_type;
struct dungeon_type {

	STR_OFFSET name; /* Name */
	STR_OFFSET text; /* Description */

	POSITION dy;
	POSITION dx;

	feat_prob floor[DUNGEON_FEAT_PROB_NUM]; /* Floor probability */
	feat_prob fill[DUNGEON_FEAT_PROB_NUM];  /* Cave wall probability */
	FEAT_IDX outer_wall;                        /* Outer wall tile */
	FEAT_IDX inner_wall;                        /* Inner wall tile */
	FEAT_IDX stream1;                           /* stream tile */
	FEAT_IDX stream2;                           /* stream tile */

	DEPTH mindepth;         /* Minimal depth */
	DEPTH maxdepth;         /* Maximal depth */
	PLAYER_LEVEL min_plev;         /* Minimal plev needed to enter -- it's an anti-cheating mesure */
	BIT_FLAGS16 pit;
	BIT_FLAGS16 nest;
	BIT_FLAGS8 mode; /* Mode of combinaison of the monster flags */

	int min_m_alloc_level;	/* Minimal number of monsters per level */
	int max_m_alloc_chance;	/* There is a 1/max_m_alloc_chance chance per round of creating a new monster */

	BIT_FLAGS flags1;		/* Flags 1 */

	BIT_FLAGS mflags1;		/* The monster flags that are allowed */
	BIT_FLAGS mflags2;
	BIT_FLAGS mflags3;
	BIT_FLAGS mflags4;
	BIT_FLAGS mflags7;
	BIT_FLAGS mflags8;
	BIT_FLAGS mflags9;
	BIT_FLAGS mflagsr;

	BIT_FLAGS m_a_ability_flags1;
	BIT_FLAGS m_a_ability_flags2;
	BIT_FLAGS m_a_ability_flags3;
	BIT_FLAGS m_a_ability_flags4;

	char r_char[5];		/* Monster race allowed */
	KIND_OBJECT_IDX final_object;	/* The object you'll find at the bottom */
	ARTIFACT_IDX final_artifact;	/* The artifact you'll find at the bottom */
	MONRACE_IDX final_guardian;	/* The artifact's guardian. If an artifact is specified, then it's NEEDED */

	PROB special_div;	/* % of monsters affected by the flags/races allowed, to add some variety */
	int tunnel_percent;
	int obj_great;
	int obj_good;
};

extern DUNGEON_IDX max_d_idx;
extern dungeon_type *d_info;
extern char *d_name;
extern char *d_text;