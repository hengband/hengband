#pragma once

#include "system/angband.h"
#include <string>

#define GINOU_SUDE 0
#define GINOU_NITOURYU 1
#define GINOU_RIDING 2
#define GINOU_SHIELD 3
#define GINOU_TEMPMAX 4

/* Proficiency level */
#define EXP_LEVEL_UNSKILLED 0
#define EXP_LEVEL_BEGINNER 1
#define EXP_LEVEL_SKILLED 2
#define EXP_LEVEL_EXPERT 3
#define EXP_LEVEL_MASTER 4

/* Proficiency of weapons and misc. skills (except riding) */
#define WEAPON_EXP_UNSKILLED 0
#define WEAPON_EXP_BEGINNER 4000
#define WEAPON_EXP_SKILLED 6000
#define WEAPON_EXP_EXPERT 7000
#define WEAPON_EXP_MASTER 8000

/* Proficiency of riding */
#define RIDING_EXP_UNSKILLED 0
#define RIDING_EXP_BEGINNER 500
#define RIDING_EXP_SKILLED 2000
#define RIDING_EXP_EXPERT 5000
#define RIDING_EXP_MASTER 8000

/* Proficiency of spells */
#define SPELL_EXP_UNSKILLED 0
#define SPELL_EXP_BEGINNER 900
#define SPELL_EXP_SKILLED 1200
#define SPELL_EXP_EXPERT 1400
#define SPELL_EXP_MASTER 1600

extern const concptr exp_level_str[5];

/*
 * Information about "skill"
 */
typedef struct skill_table {
    SUB_EXP w_start[5][64]{}; /* start weapon exp */
    SUB_EXP w_max[5][64]{}; /* max weapon exp */
    SUB_EXP s_start[10]{}; /* start skill */
    SUB_EXP s_max[10]{}; /* max skill */
} skill_table;

extern skill_table *s_info;
