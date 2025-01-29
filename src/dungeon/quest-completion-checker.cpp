#include "dungeon/quest-completion-checker.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "grid/grid.h"
#include "object-enchant/item-apply-magic.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "view/display-messages.h"
#include <algorithm>

QuestCompletionChecker::QuestCompletionChecker(PlayerType *player_ptr, MonsterEntity *m_ptr)
    : player_ptr(player_ptr)
    , m_ptr(m_ptr)
    , quest_idx(QuestId::NONE)
{
}

/*!
 * @brief 特定の敵を倒した際にクエスト達成処理 /
 * Check for "Quest" completion when a quest monster is killed or charmed.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr 撃破したモンスターの構造体参照ポインタ
 */
void QuestCompletionChecker::complete()
{
    this->set_quest_idx();
    auto create_stairs = false;
    auto reward = false;
    auto &quests = QuestList::get_instance();
    if (inside_quest(this->quest_idx) && (quests.get_quest(this->quest_idx).status == QuestStatusType::TAKEN)) {
        this->q_ptr = &quests.get_quest(this->quest_idx);
        auto [tmp_create_stairs, tmp_reward] = this->switch_completion();
        create_stairs = tmp_create_stairs;
        reward = tmp_reward;
    }

    auto pos = this->make_stairs(create_stairs);
    if (!reward) {
        return;
    }

    this->make_reward(pos);
}

static bool check_quest_completion(PlayerType *player_ptr, const QuestType &quest, MonsterEntity *m_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (quest.status != QuestStatusType::TAKEN) {
        return false;
    }

    if (any_bits(quest.flags, QUEST_FLAG_PRESET)) {
        return false;
    }

    if ((quest.level != floor_ptr->dun_level)) {
        return false;
    }

    if ((quest.type == QuestKindType::FIND_ARTIFACT) || (quest.type == QuestKindType::FIND_EXIT)) {
        return false;
    }

    auto kill_them_all = quest.type == QuestKindType::KILL_NUMBER;
    kill_them_all |= quest.type == QuestKindType::TOWER;
    kill_them_all |= quest.type == QuestKindType::KILL_ALL;
    if (kill_them_all) {
        return true;
    }

    auto is_target = (quest.type == QuestKindType::RANDOM) && (quest.r_idx == m_ptr->r_idx);
    if ((quest.type == QuestKindType::KILL_LEVEL) || is_target) {
        return true;
    }

    return false;
}

void QuestCompletionChecker::set_quest_idx()
{
    const auto &floor = *this->player_ptr->current_floor_ptr;
    const auto &quests = QuestList::get_instance();
    this->quest_idx = floor.quest_number;
    if (inside_quest(this->quest_idx)) {
        return;
    }
    auto q = std::find_if(quests.rbegin(), quests.rend(), [this](auto q) { return check_quest_completion(this->player_ptr, q.second, this->m_ptr); });

    if (q != quests.rend()) {
        this->quest_idx = q->first;
    } else {
        this->quest_idx = QuestId::NONE;
    }
}

std::tuple<bool, bool> QuestCompletionChecker::switch_completion()
{
    switch (this->q_ptr->type) {
    case QuestKindType::KILL_NUMBER:
        this->complete_kill_number();
        return std::make_tuple(false, false);
    case QuestKindType::KILL_ALL:
        this->complete_kill_all();
        return std::make_tuple(false, false);
    case QuestKindType::KILL_LEVEL:
    case QuestKindType::RANDOM:
        return this->complete_random();
    case QuestKindType::TOWER:
        this->complete_tower();
        return std::make_tuple(false, false);
    default:
        return std::make_tuple(false, false);
    }
}

void QuestCompletionChecker::complete_kill_number()
{
    this->q_ptr->cur_num++;
    if (this->q_ptr->cur_num >= this->q_ptr->num_mon) {
        complete_quest(this->player_ptr, this->quest_idx);
        this->q_ptr->cur_num = 0;
    }
}

void QuestCompletionChecker::complete_kill_all()
{
    if (!this->m_ptr->is_hostile() || (this->count_all_hostile_monsters() != 1)) {
        return;
    }

    if (any_bits(this->q_ptr->flags, QUEST_FLAG_SILENT)) {
        this->q_ptr->status = QuestStatusType::FINISHED;
    } else {
        complete_quest(this->player_ptr, this->quest_idx);
    }
}

std::tuple<bool, bool> QuestCompletionChecker::complete_random()
{
    if (this->q_ptr->r_idx != this->m_ptr->r_idx) {
        return std::make_tuple(false, false);
    }

    this->q_ptr->cur_num++;
    if (this->q_ptr->cur_num < this->q_ptr->max_num) {
        return std::make_tuple(false, false);
    }

    complete_quest(this->player_ptr, this->quest_idx);
    auto create_stairs = false;
    if (none_bits(this->q_ptr->flags, QUEST_FLAG_PRESET)) {
        create_stairs = true;
        this->player_ptr->current_floor_ptr->quest_number = QuestId::NONE;
    }

    if ((this->quest_idx == QuestId::OBERON) || (this->quest_idx == QuestId::SERPENT)) {
        this->q_ptr->status = QuestStatusType::FINISHED;
    }

    auto reward = false;
    if (this->q_ptr->type == QuestKindType::RANDOM) {
        reward = true;
        this->q_ptr->status = QuestStatusType::FINISHED;
    }

    return std::make_tuple(create_stairs, reward);
}

void QuestCompletionChecker::complete_tower()
{
    const auto &quests = QuestList::get_instance();
    if (!this->m_ptr->is_hostile()) {
        return;
    }

    if (this->count_all_hostile_monsters() != 1) {
        return;
    }

    this->q_ptr->status = QuestStatusType::STAGE_COMPLETED;
    auto is_tower_completed = quests.get_quest(QuestId::TOWER1).status == QuestStatusType::STAGE_COMPLETED;
    is_tower_completed &= quests.get_quest(QuestId::TOWER2).status == QuestStatusType::STAGE_COMPLETED;
    is_tower_completed &= quests.get_quest(QuestId::TOWER3).status == QuestStatusType::STAGE_COMPLETED;
    if (is_tower_completed) {
        complete_quest(this->player_ptr, QuestId::TOWER1);
    }
}

/*!
 * @brief 現在フロアに残っている敵モンスターの数を返す /
 * @return 現在の敵モンスターの数
 */
int QuestCompletionChecker::count_all_hostile_monsters()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto number_mon = 0;
    for (auto x = 0; x < floor_ptr->width; ++x) {
        for (auto y = 0; y < floor_ptr->height; ++y) {
            auto m_idx = floor_ptr->grid_array[y][x].m_idx;
            if ((m_idx > 0) && floor_ptr->m_list[m_idx].is_hostile()) {
                ++number_mon;
            }
        }
    }

    return number_mon;
}

Pos2D QuestCompletionChecker::make_stairs(const bool create_stairs)
{
    auto m_pos = this->m_ptr->get_position();
    if (!create_stairs) {
        return m_pos;
    }

    auto &floor = *this->player_ptr->current_floor_ptr;
    auto *g_ptr = &floor.get_grid(m_pos);
    while (floor.has_terrain_characteristics(m_pos, TerrainCharacteristics::PERMANENT) || !g_ptr->o_idx_list.empty() || g_ptr->is_object()) {
        m_pos = scatter(this->player_ptr, m_pos, 1, PROJECT_NONE);
        g_ptr = &floor.get_grid(m_pos);
    }

    msg_print(_("魔法の階段が現れた...", "A magical staircase appears..."));
    set_terrain_id_to_grid(this->player_ptr, m_pos, TerrainTag::DOWN_STAIR);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
    return m_pos;
}

void QuestCompletionChecker::make_reward(const Pos2D pos)
{
    auto dun_level = this->player_ptr->current_floor_ptr->dun_level;
    for (auto i = 0; i < (dun_level / 15) + 1; i++) {
        ItemEntity item;
        while (true) {
            item.wipe();
            const auto &monrace = this->m_ptr->get_monrace();
            (void)make_object(this->player_ptr, &item, AM_GOOD | AM_GREAT, monrace.level);
            if (!this->check_quality(item)) {
                continue;
            }

            (void)drop_near(this->player_ptr, &item, pos);
            break;
        }
    }
}

/*!
 * @brief ランダムクエスト報酬の品質をチェックする
 * @param item 生成された高級品への参照
 * @return 十分な品質か否か
 * @details 以下のものを弾く
 * 1. 呪われれた装備品
 * 2. 固定アーティファクト以外の矢弾
 * 3. 穴掘りエゴの装備品
 */
bool QuestCompletionChecker::check_quality(ItemEntity &item)
{
    auto is_good_reward = !item.is_cursed();
    is_good_reward &= !item.is_ammo() || (item.is_ammo() && item.is_fixed_artifact());
    is_good_reward &= item.ego_idx != EgoType::DIGGING;
    return is_good_reward;
}
