﻿#include "dungeon/quest-completion-checker.h"
#include "core/player-update-types.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-ego.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

QuestCompletionChecker::QuestCompletionChecker(PlayerType *player_ptr, monster_type *m_ptr)
    : player_ptr(player_ptr)
    , m_ptr(m_ptr)
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
    if ((this->quest_idx > 0) && (quest[this->quest_idx].status == QuestStatusType::TAKEN)) {
        this->q_ptr = &quest[this->quest_idx];
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

void QuestCompletionChecker::set_quest_idx()
{
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    this->quest_idx = floor_ptr->inside_quest;
    if (this->quest_idx > 0) {
        return;
    }

    short i;
    for (i = max_q_idx - 1; i > 0; i--) {
        auto *const quest_ptr = &quest[i];
        if (quest_ptr->status != QuestStatusType::TAKEN) {
            continue;
        }

        if (any_bits(quest_ptr->flags, QUEST_FLAG_PRESET)) {
            continue;
        }

        if ((quest_ptr->level != floor_ptr->dun_level) && (quest_ptr->type != QuestKindType::KILL_ANY_LEVEL)) {
            continue;
        }

        if ((quest_ptr->type == QuestKindType::FIND_ARTIFACT) || (quest_ptr->type == QuestKindType::FIND_EXIT)) {
            continue;
        }

        auto kill_them_all = quest_ptr->type == QuestKindType::KILL_NUMBER;
        kill_them_all |= quest_ptr->type == QuestKindType::TOWER;
        kill_them_all |= quest_ptr->type == QuestKindType::KILL_ALL;
        if (kill_them_all) {
            break;
        }

        auto is_target = (quest_ptr->type == QuestKindType::RANDOM) && (quest_ptr->r_idx == this->m_ptr->r_idx);
        if ((quest_ptr->type == QuestKindType::KILL_LEVEL) || (quest_ptr->type == QuestKindType::KILL_ANY_LEVEL) || is_target) {
            break;
        }
    }

    this->quest_idx = i;
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
    case QuestKindType::KILL_ANY_LEVEL:
        this->complete_kill_any_level();
        return std::make_tuple(false, false);
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
    if (!is_hostile(this->m_ptr) || (this->count_all_hostile_monsters() != 1)) {
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
        this->player_ptr->current_floor_ptr->inside_quest = 0;
    }

    if ((this->quest_idx == QUEST_OBERON) || (this->quest_idx == QUEST_SERPENT)) {
        this->q_ptr->status = QuestStatusType::FINISHED;
    }

    auto reward = false;
    if (this->q_ptr->type == QuestKindType::RANDOM) {
        reward = true;
        this->q_ptr->status = QuestStatusType::FINISHED;
    }

    return std::make_tuple(create_stairs, reward);
}

void QuestCompletionChecker::complete_kill_any_level()
{
    this->q_ptr->cur_num++;
    if (this->q_ptr->cur_num >= this->q_ptr->max_num) {
        complete_quest(this->player_ptr, this->quest_idx);
        this->q_ptr->cur_num = 0;
    }
}

void QuestCompletionChecker::complete_tower()
{
    if (!is_hostile(this->m_ptr)) {
        return;
    }

    if (this->count_all_hostile_monsters() != 1) {
        return;
    }

    this->q_ptr->status = QuestStatusType::STAGE_COMPLETED;
    auto is_tower_completed = quest[QUEST_TOWER1].status == QuestStatusType::STAGE_COMPLETED;
    is_tower_completed &= quest[QUEST_TOWER2].status == QuestStatusType::STAGE_COMPLETED;
    is_tower_completed &= quest[QUEST_TOWER3].status == QuestStatusType::STAGE_COMPLETED;
    if (is_tower_completed) {
        complete_quest(this->player_ptr, QUEST_TOWER1);
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
            if ((m_idx > 0) && is_hostile(&floor_ptr->m_list[m_idx])) {
                ++number_mon;
            }
        }
    }

    return number_mon;
}

Pos2D QuestCompletionChecker::make_stairs(const bool create_stairs)
{
    auto y = this->m_ptr->fy;
    auto x = this->m_ptr->fx;
    if (!create_stairs) {
        return Pos2D(y, x);
    }

    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    while (cave_has_flag_bold(floor_ptr, y, x, FF::PERMANENT) || !g_ptr->o_idx_list.empty() || g_ptr->is_object()) {
        int ny;
        int nx;
        scatter(this->player_ptr, &ny, &nx, y, x, 1, PROJECT_NONE);
        y = ny;
        x = nx;
        g_ptr = &floor_ptr->grid_array[y][x];
    }

    msg_print(_("魔法の階段が現れた...", "A magical staircase appears..."));
    cave_set_feat(this->player_ptr, y, x, feat_down_stair);
    set_bits(this->player_ptr->update, PU_FLOW);
    return Pos2D(y, x);
}

void QuestCompletionChecker::make_reward(const Pos2D pos)
{
    auto dun_level = this->player_ptr->current_floor_ptr->dun_level;
    for (auto i = 0; i < (dun_level / 15) + 1; i++) {
        object_type item;
        while (true) {
            item.wipe();
            auto &r_ref = r_info[this->m_ptr->r_idx];
            (void)make_object(this->player_ptr, &item, AM_GOOD | AM_GREAT, r_ref.level);
            if (!this->check_quality(item)) {
                continue;
            }

            (void)drop_near(this->player_ptr, &item, -1, pos.y, pos.x);
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
bool QuestCompletionChecker::check_quality(object_type &item)
{
    auto is_good_reward = !item.is_cursed();
    is_good_reward &= !item.is_ammo() || (item.is_ammo() && item.is_fixed_artifact());
    is_good_reward &= item.name2 != EGO_DIGGING;
    return is_good_reward;
}
