#pragma once

#include "monster/monster-timed-effect-types.h"
#include <map>
#include <string>

/*
 * Some screen locations for various display routines
 * Currently, row 8 and 15 are the only "blank" rows.
 * That leaves a "border" around the "stat" values.
 */

#define ROW_RACE 1
#define COL_RACE 0 /* <race name> */

#define ROW_TITLE 2
#define COL_TITLE 0 /* <title> or <mode> */

#define ROW_LEVEL 3
#define COL_LEVEL 0 /* "LEVEL xxxxxx" */

#define ROW_EXP 4
#define COL_EXP 0 /* "EXP xxxxxxxx" */

#define ROW_GOLD 5
#define COL_GOLD 0 /* "AU xxxxxxxxx" */

#define ROW_EQUIPPY 6
#define COL_EQUIPPY 0 /* equippy chars */

#define ROW_STAT 7
#define COL_STAT 0 /* "xxx   xxxxxx" */

#define ROW_AC 13
#define COL_AC 0 /* "Cur AC xxxxx" */

#define ROW_HPMP 14
#define COL_HPMP 0

#define ROW_CURHP 14
#define COL_CURHP 0 /* "Cur HP xxxxx" */

#define ROW_CURSP 15
#define COL_CURSP 0 /* "Cur SP xxxxx" */

#define ROW_RIDING_INFO 16
#define COL_RIDING_INFO 0 /* "xxxxxxxxxxxx" */

#define ROW_INFO 17
#define COL_INFO 0 /* "xxxxxxxxxxxx" */

#define ROW_CUT (-6)
#define COL_CUT 0 /* <cut> */

#define ROW_STUN (-5)
#define COL_STUN 0 /* <stun> */

#define ROW_HUNGRY (-4)
#define COL_HUNGRY 0 /* "Weak" / "Hungry" / "Full" / "Gorged" */

#define ROW_STATE (-4)
#define COL_STATE 7 /* <state> */

#define ROW_DAY (-3)
#define COL_DAY 0 /* day */

#define ROW_DUNGEON (-2)
#define COL_DUNGEON 0 /* dungeon */

#define ROW_SPEED (-1)
#define COL_SPEED (-24) /* "Slow (-NN)" or "Fast (+NN)" */

#define ROW_STUDY (-1)
#define COL_STUDY (-13) /* "Study" */

#define ROW_DEPTH (-1)
#define COL_DEPTH (-8) /* "Lev NNN" / "NNNN ft" */

#define ROW_STATBAR (-1)
#define COL_STATBAR 0
#define MAX_COL_STATBAR (-26)

extern const std::map<monster_timed_effect_type, std::string> effect_type_to_label;
