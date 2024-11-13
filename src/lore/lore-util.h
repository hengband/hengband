#pragma once

#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-aura-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-behavior-flags.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-drop-flags.h"
#include "monster-race/race-feature-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-kind-flags.h"
#include "monster-race/race-misc-flags.h"
#include "monster-race/race-special-flags.h"
#include "monster-race/race-visual-flags.h"
#include "system/angband.h"
#include "term/term-color-types.h"
#include "util/flag-group.h"
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

enum class MonraceId : short;
enum class MonsterSex;

enum monster_lore_mode {
    MONSTER_LORE_NONE,
    MONSTER_LORE_NORMAL,
    MONSTER_LORE_RESEARCH,
    MONSTER_LORE_DEBUG
};

class MonraceDefinition;
struct lore_msg {
    lore_msg(std::string_view msg, byte color = TERM_WHITE);
    std::string msg;
    byte color;
};

struct lore_type {
    lore_type(MonraceId r_idx, monster_lore_mode mode);

#ifndef JP
    bool sin = false;
#endif

    bool know_everything = false;
    bool old = false;
    int count = 0;
    bool shoot = false;
    bool rocket = false;
    std::vector<lore_msg> lore_msgs;
    bool breath = false;
    bool magic = false;
    int drop_quantity = 0;
    concptr drop_quality = "";
    concptr p = "";
    byte pc = 0;
    concptr q = "";
    byte qc = 0;

    MonraceId r_idx;
    BIT_FLAGS mode;
    MonsterSex msex;
    RaceBlowMethodType method;

    bool nightmare;
    MonraceDefinition *r_ptr;
    byte speed;
    ITEM_NUMBER drop_gold;
    ITEM_NUMBER drop_item;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    EnumClassFlagGroup<MonsterAuraType> aura_flags;
    EnumClassFlagGroup<MonsterBehaviorType> behavior_flags;
    EnumClassFlagGroup<MonsterVisualType> visual_flags;
    EnumClassFlagGroup<MonsterKindType> kind_flags;
    EnumClassFlagGroup<MonsterResistanceType> resistance_flags;
    EnumClassFlagGroup<MonsterDropType> drop_flags;
    EnumClassFlagGroup<MonsterFeatureType> feature_flags;
    EnumClassFlagGroup<MonsterBrightnessType> brightness_flags;
    EnumClassFlagGroup<MonsterSpecialType> special_flags;
    EnumClassFlagGroup<MonsterMiscType> misc_flags;

    bool has_reinforce() const;
    bool is_details_known() const;

    std::optional<std::vector<lore_msg>> build_kill_unique_description() const;
    std::string build_revenge_description(bool has_defeated) const;
    std::vector<lore_msg> build_speed_description() const;

private:
    std::vector<lore_msg> build_random_movement_description() const;
};

using hook_c_roff_pf = void (*)(TERM_COLOR attr, std::string_view str);
extern hook_c_roff_pf hook_c_roff;

void hooked_roff(std::string_view str);

enum WHO_WORD_TYPE { WHO = 0,
    WHOSE = 1,
    WHOM = 2 };
using who_word_definition = std::unordered_map<WHO_WORD_TYPE, const std::unordered_map<bool, const std::unordered_map<MonsterSex, std::string>>>;

class Who {
public:
    static const who_word_definition words;

    /*!
     * @brief 三人称主格を取得(単数のみ)
     * @param msex モンスターの性別
     * @return 主語
     */
    static std::string who(MonsterSex msex)
    {
        return who(msex, false);
    }

    /*!
     * @brief 三人称主格を取得
     * @param msex モンスターの性別
     * @param multi 複数かどうか
     * @return 主語
     */
    static std::string who(MonsterSex msex, bool multi)
    {
        return words.at(WHO_WORD_TYPE::WHO).at(multi).at(msex).data();
    }

    /*!
     * @brief 三人称所有格を取得(単数のみ)
     * @param msex モンスターの性別
     * @return 所有格
     */
    static std::string whose(MonsterSex msex)
    {
        return whose(msex, false);
    }

    /*!
     * @brief 三人称所有格を取得
     * @param msex モンスターの性別
     * @param multi 複数かどうか
     * @return 所有格
     */
    static std::string whose(MonsterSex msex, bool multi)
    {
        return words.at(WHO_WORD_TYPE::WHOSE).at(multi).at(msex).data();
    }

    /*!
     * @brief 三人称目的格を取得(単数のみ)
     * @param msex モンスターの性別
     * @return 目的語
     */
    static std::string whom(MonsterSex msex)
    {
        return whom(msex, false);
    }

    /*!
     * @brief 三人称目的格を取得
     * @param msex モンスターの性別
     * @param multi 複数かどうか
     * @return 目的語
     */
    static std::string whom(MonsterSex msex, bool multi)
    {
        return words.at(WHO_WORD_TYPE::WHOM).at(multi).at(msex).data();
    }
};
