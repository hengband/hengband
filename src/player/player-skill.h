#pragma once

#include "system/angband.h"

#include <array>
#include <map>
#include <string>
#include <vector>

enum skill_idx {
    SKILL_MARTIAL_ARTS = 0,
    SKILL_TWO_WEAPON = 1,
    SKILL_RIDING = 2,
    SKILL_SHIELD = 3,
    SKILL_MAX = 4,
};

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

enum class ItemKindType : short;

/*
 * Information about "skill"
 */
typedef struct skill_table {
    std::map<ItemKindType, std::array<SUB_EXP, 64>> w_start{}; /* start weapon exp */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> w_max{}; /* max weapon exp */
    SUB_EXP s_start[MAX_SKILLS]{}; /* start skill */
    SUB_EXP s_max[MAX_SKILLS]{}; /* max skill */
} skill_table;

extern std::vector<skill_table> s_info;
