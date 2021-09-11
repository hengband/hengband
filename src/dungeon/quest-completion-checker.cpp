#include "dungeon/quest-completion-checker.h"
#include "core/player-update-types.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "object-enchant/item-apply-magic.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 現在フロアに残っている敵モンスターの数を返す /
 * @return 現在の敵モンスターの数
 */
static MONSTER_NUMBER count_all_hostile_monsters(floor_type *floor_ptr)
{
    MONSTER_NUMBER number_mon = 0;
    for (POSITION x = 0; x < floor_ptr->width; ++x) {
        for (POSITION y = 0; y < floor_ptr->height; ++y) {
            MONSTER_IDX m_idx = floor_ptr->grid_array[y][x].m_idx;
            if (m_idx > 0 && is_hostile(&floor_ptr->m_list[m_idx]))
                ++number_mon;
        }
    }

    return number_mon;
}

/*!
 * @brief 特定の敵を倒した際にクエスト達成処理 /
 * Check for "Quest" completion when a quest monster is killed or charmed.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_ptr 撃破したモンスターの構造体参照ポインタ
 */
void check_quest_completion(player_type *player_ptr, monster_type *m_ptr)
{
    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    QUEST_IDX quest_num = floor_ptr->inside_quest;
    if (!quest_num) {
        QUEST_IDX i;
        for (i = max_q_idx - 1; i > 0; i--) {
            quest_type *const q_ptr = &quest[i];
            if (q_ptr->status != QUEST_STATUS_TAKEN)
                continue;

            if (q_ptr->flags & QUEST_FLAG_PRESET)
                continue;

            if ((q_ptr->level != floor_ptr->dun_level) && (q_ptr->type != QUEST_TYPE_KILL_ANY_LEVEL))
                continue;

            if ((q_ptr->type == QUEST_TYPE_FIND_ARTIFACT) || (q_ptr->type == QUEST_TYPE_FIND_EXIT))
                continue;

            if ((q_ptr->type == QUEST_TYPE_KILL_NUMBER) || (q_ptr->type == QUEST_TYPE_TOWER) || (q_ptr->type == QUEST_TYPE_KILL_ALL))
                break;

            if (((q_ptr->type == QUEST_TYPE_KILL_LEVEL) || (q_ptr->type == QUEST_TYPE_KILL_ANY_LEVEL) || (q_ptr->type == QUEST_TYPE_RANDOM))
                && (q_ptr->r_idx == m_ptr->r_idx))
                break;
        }

        quest_num = i;
    }

    bool create_stairs = false;
    bool reward = false;
    if (quest_num && (quest[quest_num].status == QUEST_STATUS_TAKEN)) {
        quest_type *const q_ptr = &quest[quest_num];
        switch (q_ptr->type) {
        case QUEST_TYPE_KILL_NUMBER: {
            q_ptr->cur_num++;
            if (q_ptr->cur_num >= q_ptr->num_mon) {
                complete_quest(player_ptr, quest_num);
                q_ptr->cur_num = 0;
            }

            break;
        }
        case QUEST_TYPE_KILL_ALL: {
            if (!is_hostile(m_ptr) || count_all_hostile_monsters(floor_ptr) != 1)
                break;

            if (q_ptr->flags & QUEST_FLAG_SILENT) {
                q_ptr->status = QUEST_STATUS_FINISHED;
            } else {
                complete_quest(player_ptr, quest_num);
            }

            break;
        }
        case QUEST_TYPE_KILL_LEVEL:
        case QUEST_TYPE_RANDOM: {
            if (q_ptr->r_idx != m_ptr->r_idx)
                break;

            q_ptr->cur_num++;
            if (q_ptr->cur_num < q_ptr->max_num)
                break;

            complete_quest(player_ptr, quest_num);
            if (!(q_ptr->flags & QUEST_FLAG_PRESET)) {
                create_stairs = true;
                floor_ptr->inside_quest = 0;
            }

            if ((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT))
                q_ptr->status = QUEST_STATUS_FINISHED;

            if (q_ptr->type == QUEST_TYPE_RANDOM) {
                reward = true;
                q_ptr->status = QUEST_STATUS_FINISHED;
            }

            break;
        }
        case QUEST_TYPE_KILL_ANY_LEVEL: {
            q_ptr->cur_num++;
            if (q_ptr->cur_num >= q_ptr->max_num) {
                complete_quest(player_ptr, quest_num);
                q_ptr->cur_num = 0;
            }

            break;
        }
        case QUEST_TYPE_TOWER: {
            if (!is_hostile(m_ptr))
                break;

            if (count_all_hostile_monsters(floor_ptr) == 1) {
                q_ptr->status = QUEST_STATUS_STAGE_COMPLETED;

                if ((quest[QUEST_TOWER1].status == QUEST_STATUS_STAGE_COMPLETED) && (quest[QUEST_TOWER2].status == QUEST_STATUS_STAGE_COMPLETED)
                    && (quest[QUEST_TOWER3].status == QUEST_STATUS_STAGE_COMPLETED)) {

                    complete_quest(player_ptr, QUEST_TOWER1);
                }
            }

            break;
        }
        }
    }

    if (create_stairs) {
        POSITION ny, nx;
        auto *g_ptr = &floor_ptr->grid_array[y][x];
        while (cave_has_flag_bold(floor_ptr, y, x, FF::PERMANENT) || !g_ptr->o_idx_list.empty() || g_ptr->is_object()) {
            scatter(player_ptr, &ny, &nx, y, x, 1, PROJECT_NONE);
            y = ny;
            x = nx;
            g_ptr = &floor_ptr->grid_array[y][x];
        }

        msg_print(_("魔法の階段が現れた...", "A magical staircase appears..."));
        cave_set_feat(player_ptr, y, x, feat_down_stair);
        player_ptr->update |= PU_FLOW;
    }

    if (!reward)
        return;

    object_type forge;
    object_type *o_ptr;
    for (int i = 0; i < (floor_ptr->dun_level / 15) + 1; i++) {
        o_ptr = &forge;
        o_ptr->wipe();
        make_object(player_ptr, o_ptr, AM_GOOD | AM_GREAT);
        (void)drop_near(player_ptr, o_ptr, -1, y, x);
    }
}
