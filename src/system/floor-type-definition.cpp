#include "system/floor-type-definition.h"
#include "dungeon/quest.h"
#include "game-option/birth-options.h"
#include "system/angband-system.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-range.h"

FloorType::FloorType()
    : quest_number(QuestId::NONE)
{
}

Grid &FloorType::get_grid(const Pos2D pos)
{
    return this->grid_array[pos.y][pos.x];
}

const Grid &FloorType::get_grid(const Pos2D pos) const
{
    return this->grid_array[pos.y][pos.x];
}

bool FloorType::is_in_underground() const
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
    const auto &quests = QuestList::get_instance();
    for (auto quest_id : EnumRangeInclusive(QuestId::RANDOM_QUEST1, QuestId::RANDOM_QUEST10)) {
        const auto &quest = quests.get_quest(quest_id);
        auto is_random_quest = (quest.type == QuestKindType::RANDOM);
        is_random_quest &= (quest.status == QuestStatusType::TAKEN);
        is_random_quest &= (quest.level == level);
        is_random_quest &= (quest.dungeon == DUNGEON_ANGBAND);
        if (is_random_quest) {
            return quest_id;
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
    const auto &quests = QuestList::get_instance();
    if (this->is_in_quest()) {
        return this->quest_number;
    }

    const auto level = this->dun_level + bonus;
    for (const auto &[quest_id, quest] : quests) {
        if (quest.status != QuestStatusType::TAKEN) {
            continue;
        }

        auto depth_quest = (quest.type == QuestKindType::KILL_LEVEL);
        depth_quest &= !(quest.flags & QUEST_FLAG_PRESET);
        depth_quest &= (quest.level == level);
        depth_quest &= (quest.dungeon == this->dungeon_idx);
        if (depth_quest) {
            return quest_id;
        }
    }

    return this->get_random_quest_id(level);
}

/*
 * @brief 与えられた座標のグリッドがLOSフラグを持つかを調べる
 * @param pos 座標
 * @return LOSフラグを持つか否か
 */
bool FloorType::has_los(const Pos2D pos) const
{
    return this->get_grid(pos).has_los();
}

/*!
 * @brief 特別なフロアにいるかを判定する
 * @return 固定クエスト、アリーナ、モンスター闘技場のいずれかならばtrue
 */
bool FloorType::is_special() const
{
    auto is_in_fixed_quest = this->is_in_quest();
    is_in_fixed_quest &= !inside_quest(this->get_random_quest_id());
    return is_in_fixed_quest || this->inside_arena || AngbandSystem::get_instance().is_phase_out();
}

/*!
 * @brief テレポート・レベル無効フロアの判定
 * @param to_player プレイヤーを対象としているか否か
 * @return テレポート・レベルが不可能ならばtrue
 */
bool FloorType::can_teleport_level(bool to_player) const
{
    auto is_invalid_floor = to_player;
    is_invalid_floor &= inside_quest(this->get_quest_id()) || (this->dun_level >= this->get_dungeon_definition().maxdepth);
    is_invalid_floor &= this->dun_level >= 1;
    is_invalid_floor &= ironman_downward;
    return this->is_special() || is_invalid_floor;
}

/*!
 * @brief 魔法の笛でペットを呼び寄せる順番を決める
 * @param index1 フロア内のモンスターインデックス1
 * @param index2 フロア内のモンスターインデックス2
 * @return index1の優先度が高いならtrue、低いならfalse
 */
bool FloorType::order_pet_whistle(short index1, short index2) const
{
    const auto &monster1 = this->m_list[index1];
    const auto &monster2 = this->m_list[index2];
    const auto is_ordered = monster1.order_pet_whistle(monster2);
    if (is_ordered) {
        return *is_ordered;
    }

    return index1 < index2;
}

/*!
 * @brief ペットを開放する順番を決める
 * @param index1 フロア内のモンスターインデックス1
 * @param index2 フロア内のモンスターインデックス2
 * @return index1の優先度が高いならtrue、低いならfalse
 */
bool FloorType::order_pet_dismission(short index1, short index2, short riding_index) const
{
    const auto &monster1 = this->m_list[index1];
    const auto &monster2 = this->m_list[index2];
    if (index1 == riding_index) {
        return true;
    }

    if (index2 == riding_index) {
        return false;
    }

    const auto is_ordered = monster1.order_pet_dismission(monster2);
    if (is_ordered) {
        return *is_ordered;
    }

    return index1 < index2;
}
