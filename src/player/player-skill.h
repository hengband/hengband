#pragma once

#include "system/angband.h"

#include "util/enum-range.h"

#include <array>
#include <map>
#include <string>
#include <vector>

enum class PlayerSkillKindType {
    MARTIAL_ARTS = 0,
    TWO_WEAPON = 1,
    RIDING = 2,
    SHIELD = 3,
    MAX,
};

constexpr auto PLAYER_SKILL_KIND_TYPE_RANGE = EnumRange(PlayerSkillKindType::MARTIAL_ARTS, PlayerSkillKindType::SHIELD);

/* Proficiency level */
#define EXP_LEVEL_UNSKILLED 0
#define EXP_LEVEL_BEGINNER 1
#define EXP_LEVEL_SKILLED 2
#define EXP_LEVEL_EXPERT 3
#define EXP_LEVEL_MASTER 4

extern const concptr exp_level_str[5];

enum class ItemKindType : short;

/*
 * Information about "skill"
 */
typedef struct skill_table {
    std::map<ItemKindType, std::array<SUB_EXP, 64>> w_start{}; /* start weapon exp */
    std::map<ItemKindType, std::array<SUB_EXP, 64>> w_max{}; /* max weapon exp */
    std::map<PlayerSkillKindType, SUB_EXP> s_start{}; /* start skill */
    std::map<PlayerSkillKindType, SUB_EXP> s_max{}; /* max skill */
} skill_table;

extern std::vector<skill_table> s_info;

struct monster_race;
struct object_type;
struct player_type;

class PlayerSkill {
public:
    PlayerSkill(player_type *player_ptr);

    static SUB_EXP weapon_exp_at(int level);
    static SUB_EXP spell_exp_at(int level);
    static bool valid_weapon_exp(int weapon_exp);
    static int weapon_exp_level(int weapon_exp);
    static int riding_exp_level(int riding_exp);
    static int spell_exp_level(int spell_exp);
    static concptr skill_name(PlayerSkillKindType skill);

    void gain_melee_weapon_exp(const object_type *o_ptr);
    void gain_range_weapon_exp(const object_type *o_ptr);
    void gain_martial_arts_skill_exp();
    void gain_two_weapon_skill_exp();
    void gain_riding_skill_exp_on_melee_attack(const monster_race *r_ptr);
    void gain_riding_skill_exp_on_range_attack();
    void gain_riding_skill_exp_on_fall_off_check(HIT_POINT dam);
    void gain_spell_skill_exp(int realm, int spell_idx);
    void gain_continuous_spell_skill_exp(int realm, int spell_idx);
    int gain_spell_skill_exp_over_learning(int spell_idx);

    EXP exp_of_spell(int realm, int spell_idx) const;

private:
    player_type *player_ptr;
};
