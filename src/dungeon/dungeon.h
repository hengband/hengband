﻿#pragma once

#include <string>
#include <vector>

#include "dungeon/dungeon-flag-types.h"
#include "monster-race/race-ability-flags.h"
#include "system/angband.h"
#include "util/flag-group.h"

#define DUNGEON_FEAT_PROB_NUM 3

enum dungeon_idx {
    DUNGEON_NONE = 0,
    DUNGEON_ANGBAND = 1,
	DUNGEON_GALGALS = 2,
	DUNGEON_ORC = 3,
	DUNGEON_MAZE = 4,
	DUNGEON_DRAGON = 5,
	DUNGEON_GRAVE = 6,
	DUNGEON_WOOD = 7,
	DUNGEON_VOLCANO = 8,
	DUNGEON_HELL = 9,
	DUNGEON_HEAVEN = 10,
	DUNGEON_OCEAN = 11,
	DUNGEON_CASTLE = 12,
	DUNGEON_CTH = 13,
	DUNGEON_MOUNTAIN = 14,
	DUNGEON_GOLD = 15,
	DUNGEON_NO_MAGIC = 16,
	DUNGEON_NO_MELEE = 17,
	DUNGEON_CHAMELEON = 18,
	DUNGEON_DARKNESS = 19,
};

typedef struct feat_prob {
    FEAT_IDX feat{}; /* Feature tile */
    PERCENTAGE percent{}; /* Chance of type */
} feat_prob;

/* A structure for the != dungeon types */
typedef struct dungeon_type {

	std::string name; /* Name */
    std::string text; /* Description */

	POSITION dy{};
	POSITION dx{};

	feat_prob floor[DUNGEON_FEAT_PROB_NUM]{}; /* Floor probability */
	feat_prob fill[DUNGEON_FEAT_PROB_NUM]{};  /* Cave wall probability */
	FEAT_IDX outer_wall{};                        /* Outer wall tile */
	FEAT_IDX inner_wall{};                        /* Inner wall tile */
	FEAT_IDX stream1{};                           /* stream tile */
	FEAT_IDX stream2{};                           /* stream tile */

	DEPTH mindepth{};         /* Minimal depth */
	DEPTH maxdepth{};         /* Maximal depth */
	PLAYER_LEVEL min_plev{};         /* Minimal plev needed to enter -- it's an anti-cheating mesure */
	BIT_FLAGS16 pit{};
	BIT_FLAGS16 nest{};
	BIT_FLAGS8 mode{}; /* Mode of combinaison of the monster flags */

	int min_m_alloc_level{};	/* Minimal number of monsters per level */
	int max_m_alloc_chance{};	/* There is a 1/max_m_alloc_chance chance per round of creating a new monster */

	EnumClassFlagGroup<DF> flags{};	/* Dungeon Flags */

	BIT_FLAGS mflags1{};		/* The monster flags that are allowed */
	BIT_FLAGS mflags2{};
	BIT_FLAGS mflags3{};
	BIT_FLAGS mflags7{};
	BIT_FLAGS mflags8{};
	BIT_FLAGS mflags9{};
	BIT_FLAGS mflagsr{};

	EnumClassFlagGroup<RF_ABILITY> m_ability_flags;

	char r_char[5]{};		/* Monster race allowed */
	KIND_OBJECT_IDX final_object{};	/* The object you'll find at the bottom */
	ARTIFACT_IDX final_artifact{};	/* The artifact you'll find at the bottom */
	MONRACE_IDX final_guardian{};	/* The artifact's guardian. If an artifact is specified, then it's NEEDED */

	PROB special_div{};	/* % of monsters affected by the flags/races allowed, to add some variety */
	int tunnel_percent{};
	int obj_great{};
	int obj_good{};
} dungeon_type;

extern DEPTH *max_dlv;
extern std::vector<dungeon_type> d_info;

typedef struct player_type player_type;
enum dungeon_idx choose_dungeon(concptr note, POSITION y, POSITION x);
bool is_in_dungeon(player_type *creature_ptr);
