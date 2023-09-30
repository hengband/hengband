#include "system/floor-type-definition.h"
#include "dungeon/quest.h"
#include "system/dungeon-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "util/enum-range.h"

FloorType::FloorType()
    : quest_number(QuestId::NONE)
{
}

bool FloorType::is_in_dungeon() const
{
    return this->dun_level > 0;
}

bool FloorType::is_in_quest() const
{
    return this->quest_number != QuestId::NONE;
}

void FloorType::set_dungeon_index(short dungeon_idx_)
{
    this->dungeon_idx = dungeon_idx_;
}

void FloorType::reset_dungeon_index()
{
    this->set_dungeon_index(0);
}

dungeon_type &FloorType::get_dungeon_definition() const
{
    return dungeons_info[this->dungeon_idx];
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されているランダムクエストを探し出しIDを返す。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param level 検索対象になる階
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QuestId FloorType::get_random_quest_id(std::optional<int> level_opt) const
{
    if (this->dungeon_idx != DUNGEON_ANGBAND) {
        return QuestId::NONE;
    }

    const auto level = level_opt.value_or(this->dun_level);
    const auto &quest_list = QuestList::get_instance();
    for (auto q_idx : EnumRange(QuestId::RANDOM_QUEST1, QuestId::RANDOM_QUEST10)) {
        const auto &quest = quest_list[q_idx];
        auto is_random_quest = (quest.type == QuestKindType::RANDOM);
        is_random_quest &= (quest.status == QuestStatusType::TAKEN);
        is_random_quest &= (quest.level == level);
        is_random_quest &= (quest.dungeon == DUNGEON_ANGBAND);
        if (is_random_quest) {
            return q_idx;
        }
    }

    return QuestId::NONE;
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されている一般のクエストを探し出しIDを返す.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param bonus 検索対象になる階へのボーナス。通常0
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QuestId FloorType::get_quest_id(const int bonus) const
{
    const auto &quest_list = QuestList::get_instance();
    if (this->is_in_quest()) {
        return this->quest_number;
    }

    const auto level = this->dun_level + bonus;
    for (const auto &[q_idx, quest] : quest_list) {
        if (quest.status != QuestStatusType::TAKEN) {
            continue;
        }

        auto depth_quest = (quest.type == QuestKindType::KILL_LEVEL);
        depth_quest &= !(quest.flags & QUEST_FLAG_PRESET);
        depth_quest &= (quest.level == level);
        depth_quest &= (quest.dungeon == this->dungeon_idx);
        if (depth_quest) {
            return q_idx;
        }
    }

    return this->get_random_quest_id(level);
}
