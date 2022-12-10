#include "object-hook/hook-quest.h"
#include "artifact/fixed-art-types.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "object-enchant/trg-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "world/world.h"

/*!
 * @brief オブジェクトが賞金首の報酬対象になるかを返す
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが報酬対象になるならTRUEを返す
 */
bool object_is_bounty(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (o_ptr->bi_key.tval() != ItemKindType::CORPSE) {
        return false;
    }

    if (vanilla_town) {
        return false;
    }

    auto corpse_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
    if (player_ptr->knows_daily_bounty && (streq(monraces_info[corpse_r_idx].name.data(), monraces_info[w_ptr->today_mon].name.data()))) {
        return true;
    }

    if (corpse_r_idx == MonsterRaceId::TSUCHINOKO) {
        return true;
    }

    return MonsterRace(corpse_r_idx).is_bounty(true);
}

/*!
 * @brief オブジェクトがクエストの達成目的か否かを返す。
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す。
 */
bool object_is_quest_target(QuestId quest_idx, const ItemEntity *o_ptr)
{
    if (!inside_quest(quest_idx)) {
        return false;
    }

    const auto &quest_list = QuestList::get_instance();
    auto a_idx = quest_list[quest_idx].reward_artifact_idx;
    if (a_idx == FixedArtifactId::NONE) {
        return false;
    }

    const auto &a_ref = artifacts_info.at(a_idx);
    if (a_ref.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return false;
    }

    return o_ptr->bi_key == a_ref.bi_key;
}
