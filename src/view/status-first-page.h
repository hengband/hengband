#pragma once

#include "system/h-type.h"
#include <array>
#include <string>
#include <utility>
#include <vector>

constexpr auto ENTRY_BARE_HAND = 0;
constexpr auto ENTRY_TWO_HANDS = 1;
constexpr auto ENTRY_RIGHT_HAND1 = 2;
constexpr auto ENTRY_LEFT_HAND1 = 3;
constexpr auto ENTRY_LEFT_HAND2 = 4;
constexpr auto ENTRY_RIGHT_HAND2 = 5;
constexpr auto ENTRY_POSTURE = 6;
constexpr auto ENTRY_SHOOT_HIT_DAM = 7;
constexpr auto ENTRY_SHOOT_POWER = 8;
constexpr auto ENTRY_SPEED = 9;
constexpr auto ENTRY_BASE_AC = 10;
constexpr auto ENTRY_LEVEL = 11;
constexpr auto ENTRY_CUR_EXP = 12;
constexpr auto ENTRY_MAX_EXP = 13;
constexpr auto ENTRY_EXP_TO_ADV = 14;
constexpr auto ENTRY_GOLD = 15;
constexpr auto ENTRY_DAY = 16;
constexpr auto ENTRY_HP = 17;
constexpr auto ENTRY_SP = 18;
constexpr auto ENTRY_PLAY_TIME = 19;
constexpr auto ENTRY_SKILL_FIGHT = 20;
constexpr auto ENTRY_SKILL_SHOOT = 21;
constexpr auto ENTRY_SKILL_SAVING = 22;
constexpr auto ENTRY_SKILL_STEALTH = 23;
constexpr auto ENTRY_SKILL_PERCEP = 24;
constexpr auto ENTRY_SKILL_SEARCH = 25;
constexpr auto ENTRY_SKILL_DISARM = 26;
constexpr auto ENTRY_SKILL_DEVICE = 27;
constexpr auto ENTRY_SKILL_DIG = 45;

constexpr auto ENTRY_BLOWS = 28;
constexpr auto ENTRY_SHOTS = 29;
constexpr auto ENTRY_AVG_DMG = 30;
constexpr auto ENTRY_INFRA = 31;

constexpr auto ENTRY_NAME = 32;
constexpr auto ENTRY_SEX = 33;
constexpr auto ENTRY_RACE = 34;
constexpr auto ENTRY_CLASS = 35;
constexpr auto ENTRY_REALM = 36;
constexpr auto ENTRY_PATRON = 37;
constexpr auto ENTRY_AGE = 38;
constexpr auto ENTRY_HEIGHT = 39;
constexpr auto ENTRY_WEIGHT = 40;
constexpr auto ENTRY_SOCIAL = 41;
constexpr auto ENTRY_ALIGN = 42;

constexpr auto ENTRY_EXP_ANDR = 43;
constexpr auto ENTRY_EXP_TO_ADV_ANDR = 44;

/*!
 * @brief キャラクター画面の技能評価1行分の元値
 */
struct SkillRatingSource {
    int entry; //!< 表示項目ID (ENTRY_SKILL_*)
    int value; //!< 評価する値 (表示オプション show_actual_value が有効なら数値も表示される)
    int divisor; //!< 評価の基準比
};

constexpr auto SKILL_RATING_MAX = 9;

/*!
 * @brief キャラクター画面の技能評価の段階
 */
enum class SkillRating {
    VERY_BAD,
    BAD,
    POOR,
    FAIR,
    GOOD,
    VERY_GOOD,
    EXCELLENT,
    SUPERB,
    HEROIC,
    LEGENDARY,
};

class PlayerType;
class ItemEntity;
struct DisplayedMeleeStatistics {
    std::array<int, 3> blows; //!< 主手・副手・突然変異の表示攻撃回数
    std::array<int, 2> damage_per_round; //!< 画面に表示するラウンド平均ダメージの整数値
    bool damage_nil; //!< 平均ダメージ欄が nil! になる
};
std::array<int, 3> calc_displayed_melee_blows(PlayerType *player_ptr);
DisplayedMeleeStatistics calc_displayed_melee_statistics(PlayerType *player_ptr, const int *damage);
std::pair<SkillRating, int> classify_skill_rating(int x, int y);
std::pair<std::string, TERM_COLOR> describe_skill_rating(int x, int y);
std::vector<SkillRatingSource> calc_skill_rating_sources(PlayerType *player_ptr);
void calc_player_shot_params(PlayerType *player_ptr, ItemEntity *o_ptr, int *shots, int *shot_frac);
void calc_player_two_hands(PlayerType *player_ptr, int *damage, int *to_h);
void display_player_various(PlayerType *player_ptr);
