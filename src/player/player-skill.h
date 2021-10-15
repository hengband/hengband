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

struct monster_race;
struct object_type;
struct player_type;

class PlayerSkill {
public:
    PlayerSkill(player_type *player_ptr);

    static SUB_EXP weapon_exp_at(int level);
    static bool valid_weapon_exp(int weapon_exp);
    static int weapon_exp_level(int weapon_exp);
    static int riding_exp_level(int riding_exp);

    void gain_melee_weapon_exp(const object_type *o_ptr);
    void gain_range_weapon_exp(const object_type *o_ptr);
    void gain_martial_arts_skill_exp();
    void gain_two_weapon_skill_exp();
    void gain_riding_skill_exp_on_melee_attack(const monster_race *r_ptr);
    void gain_riding_skill_exp_on_range_attack();
    void gain_riding_skill_exp_on_fall_off_check(HIT_POINT dam);

private:
    player_type *player_ptr;
};
