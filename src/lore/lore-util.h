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
#include "monster-race/race-visual-flags.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <string>
#include <string_view>
#include <unordered_map>

enum class MonsterRaceId : int16_t;

enum monster_sex {
    MSEX_NONE = 0,
    MSEX_MALE = 1,
    MSEX_FEMALE = 2,
};

enum monster_lore_mode {
    MONSTER_LORE_NONE,
    MONSTER_LORE_NORMAL,
    MONSTER_LORE_RESEARCH,
    MONSTER_LORE_DEBUG
};

class MonsterRaceInfo;
struct lore_type {
    lore_type(MonsterRaceId r_idx, monster_lore_mode mode);

#ifndef JP
    bool sin = false;
#endif

    bool reinforce = false;
    bool know_everything = false;
    bool old = false;
    int count = 0;
    bool shoot = false;
    bool rocket = false;
    int vn = 0;
    byte color[96]{};
    concptr vp[96]{};
    char tmp_msg[96][96]{};
    bool breath = false;
    bool magic = false;
    int drop_quantity = 0;
    concptr drop_quality = "";
    concptr p = "";
    byte pc = 0;
    concptr q = "";
    byte qc = 0;

    MonsterRaceId r_idx;
    BIT_FLAGS mode;
    monster_sex msex;
    RaceBlowMethodType method;

    bool nightmare;
    MonsterRaceInfo *r_ptr;
    byte speed;
    ITEM_NUMBER drop_gold;
    ITEM_NUMBER drop_item;
    BIT_FLAGS flags1;
    BIT_FLAGS flags2;
    BIT_FLAGS flags3;
    BIT_FLAGS flags7;
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
    EnumClassFlagGroup<MonsterAuraType> aura_flags;
    EnumClassFlagGroup<MonsterBehaviorType> behavior_flags;
    EnumClassFlagGroup<MonsterVisualType> visual_flags;
    EnumClassFlagGroup<MonsterKindType> kind_flags;
    EnumClassFlagGroup<MonsterResistanceType> resistance_flags;
    EnumClassFlagGroup<MonsterDropType> drop_flags;
    EnumClassFlagGroup<MonsterFeatureType> feature_flags;
    EnumClassFlagGroup<MonsterBrightnessType> brightness_flags;
};

using hook_c_roff_pf = void (*)(TERM_COLOR attr, std::string_view str);
extern hook_c_roff_pf hook_c_roff;

void hooked_roff(std::string_view str);

enum WHO_WORD_TYPE { WHO = 0,
    WHOSE = 1,
    WHOM = 2 };
using who_word_definition = std::unordered_map<WHO_WORD_TYPE, const std::unordered_map<bool, const std::unordered_map<monster_sex, std::string>>>;

class Who {
public:
    static const who_word_definition words;

    /*!
     * @brief 三人称主格を取得(単数のみ)
     * @param msex モンスターの性別
     * @return 主語
     */
    static concptr who(monster_sex msex)
    {
        return who(msex, false);
    }

    /*!
     * @brief 三人称主格を取得
     * @param msex モンスターの性別
     * @param multi 複数かどうか
     * @return 主語
     */
    static concptr who(monster_sex msex, bool multi)
    {
        return words.at(WHO_WORD_TYPE::WHO).at(multi).at(msex).data();
    }

    /*!
     * @brief 三人称所有格を取得(単数のみ)
     * @param msex モンスターの性別
     * @return 所有格
     */
    static concptr whose(monster_sex msex)
    {
        return whose(msex, false);
    }

    /*!
     * @brief 三人称所有格を取得
     * @param msex モンスターの性別
     * @param multi 複数かどうか
     * @return 所有格
     */
    static concptr whose(monster_sex msex, bool multi)
    {
        return words.at(WHO_WORD_TYPE::WHOSE).at(multi).at(msex).data();
    }

    /*!
     * @brief 三人称目的格を取得(単数のみ)
     * @param msex モンスターの性別
     * @return 目的語
     */
    static concptr whom(monster_sex msex)
    {
        return whom(msex, false);
    }

    /*!
     * @brief 三人称目的格を取得
     * @param msex モンスターの性別
     * @param multi 複数かどうか
     * @return 目的語
     */
    static concptr whom(monster_sex msex, bool multi)
    {
        return words.at(WHO_WORD_TYPE::WHOM).at(multi).at(msex).data();
    }
};
