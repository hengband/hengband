/*!
 * @file monster-lore-snapshot.h
 * @brief モンスターの思い出の表示を決める値の写し
 * @details
 * 思い出 (倒した数、知っている耐性・呪文・打撃、警戒度など) は書き換える箇所が多く、
 * 書き換えるたびに思い出のサブウィンドウの描き直しを要求すると漏れやすい。
 * そこで表示したときの値を写しておき、今の値と比べて描き直すかを決める。
 */

#pragma once

#include "system/angband.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "util/flag-group.h"
#include <array>

class PlayerType;
class MonsterLoreSnapshot {
public:
    MonsterLoreSnapshot(const PlayerType &player, const MonraceDefinition &monrace);

    bool operator==(const MonsterLoreSnapshot &other) const = default;

private:
    MonraceId monrace_id{};
    PLAYER_LEVEL player_level = 0; //!< 経験値の説明に使う
    PLAYER_LEVEL player_max_level = 0; //!< 経験値の計算に使う
    bool is_wizard = false; //!< ウィザードモードなら種族の番号を表示する
    bool knows_everything = false; //!< cheat_know なら全ての思い出を表示する
    bool shows_summary = false; //!< show_lore_summary なら要約を表示する
    bool shows_depth_in_feet = false; //!< depth_in_feet なら出現する階をフィートで表示する
    bool has_been_seen = false; //!< オベロンと混沌のサーペントで、倒したい衝動の説明に使う
    MONSTER_NUMBER max_num = 0; //!< ユニークの生死やナズグルの残りの数の説明に使う
    MONSTER_NUMBER r_deaths = 0;
    MONSTER_NUMBER r_pkills = 0;
    MONSTER_NUMBER r_akills = 0;
    MONSTER_NUMBER r_tkills = 0;
    byte r_wake = 0;
    byte r_ignore = 0;
    bool r_can_evolve = false;
    ITEM_NUMBER r_drop_gold = 0;
    ITEM_NUMBER r_drop_item = 0;
    byte r_cast_spell = 0;
    std::array<byte, MAX_NUM_BLOWS> r_blows{};
    EnumClassFlagGroup<MonsterAbilityType> r_ability_flags;
    EnumClassFlagGroup<MonsterAuraType> r_aura_flags;
    EnumClassFlagGroup<MonsterBehaviorType> r_behavior_flags;
    EnumClassFlagGroup<MonsterResistanceType> r_resistance_flags;
    EnumClassFlagGroup<MonsterDropType> r_drop_flags;
    EnumClassFlagGroup<MonsterFeatureType> r_feature_flags;
    EnumClassFlagGroup<MonsterSpecialType> r_special_flags;
    EnumClassFlagGroup<MonsterMiscType> r_misc_flags;
};
