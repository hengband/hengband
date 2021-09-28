/*!
 *  @brief プレイヤーの移動処理 / Movement commands
 *  @date 2014/01/02
 *  @author
 *  Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 */

#include "player/player-move.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/floor-util.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "inventory/player-inventory.h"
#include "io/input-key-requester.h"
#include "mind/mind-ninja.h"
#include "monster/monster-update.h"
#include "perception/object-perception.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-kind/spells-floor.h"
#include "spell-realm/spells-song.h"
#include "spell/spell-types.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

int flow_head = 0;
int flow_tail = 0;
POSITION temp2_x[MAX_SHORT];
POSITION temp2_y[MAX_SHORT];

/*!
 * @brief 地形やその上のアイテムの隠された要素を全て明かす /
 * Search for hidden things
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象となるマスのY座標
 * @param x 対象となるマスのX座標
 */
static void discover_hidden_things(player_type *player_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->mimic && is_trap(player_ptr, g_ptr->feat)) {
        disclose_grid(player_ptr, y, x);
        msg_print(_("トラップを発見した。", "You have found a trap."));
        disturb(player_ptr, false, true);
    }

    if (is_hidden_door(player_ptr, g_ptr)) {
        msg_print(_("隠しドアを発見した。", "You have found a secret door."));
        disclose_grid(player_ptr, y, x);
        disturb(player_ptr, false, false);
    }

    for (const auto this_o_idx : g_ptr->o_idx_list) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (o_ptr->tval != TV_CHEST)
            continue;
        if (chest_traps[o_ptr->pval].none())
            continue;
        if (!o_ptr->is_known()) {
            msg_print(_("箱に仕掛けられたトラップを発見した！", "You have discovered a trap on the chest!"));
            object_known(o_ptr);
            disturb(player_ptr, false, false);
        }
    }
}

/*!
 * @brief プレイヤーの探索処理判定
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void search(player_type *player_ptr)
{
    PERCENTAGE chance = player_ptr->skill_srh;
    if (player_ptr->blind || no_lite(player_ptr))
        chance = chance / 10;

    if (player_ptr->confused || player_ptr->hallucinated)
        chance = chance / 10;

    for (DIRECTION i = 0; i < 9; ++i)
        if (randint0(100) < chance)
            discover_hidden_things(player_ptr, player_ptr->y + ddy_ddd[i], player_ptr->x + ddx_ddd[i]);
}

/*!
 * @brief 移動に伴うプレイヤーのステータス変化処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ny 移動先Y座標
 * @param nx 移動先X座標
 * @param mpe_mode 移動オプションフラグ
 * @return プレイヤーが死亡やフロア離脱を行わず、実際に移動が可能ならばTRUEを返す。
 */
bool move_player_effect(player_type *player_ptr, POSITION ny, POSITION nx, BIT_FLAGS mpe_mode)
{
    POSITION oy = player_ptr->y;
    POSITION ox = player_ptr->x;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[ny][nx];
    grid_type *oc_ptr = &floor_ptr->grid_array[oy][ox];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    feature_type *of_ptr = &f_info[oc_ptr->feat];

    if (!(mpe_mode & MPE_STAYING)) {
        MONSTER_IDX om_idx = oc_ptr->m_idx;
        MONSTER_IDX nm_idx = g_ptr->m_idx;
        player_ptr->y = ny;
        player_ptr->x = nx;
        if (!(mpe_mode & MPE_DONT_SWAP_MON)) {
            g_ptr->m_idx = om_idx;
            oc_ptr->m_idx = nm_idx;
            if (om_idx > 0) {
                monster_type *om_ptr = &floor_ptr->m_list[om_idx];
                om_ptr->fy = ny;
                om_ptr->fx = nx;
                update_monster(player_ptr, om_idx, true);
            }

            if (nm_idx > 0) {
                monster_type *nm_ptr = &floor_ptr->m_list[nm_idx];
                nm_ptr->fy = oy;
                nm_ptr->fx = ox;
                update_monster(player_ptr, nm_idx, true);
            }
        }

        lite_spot(player_ptr, oy, ox);
        lite_spot(player_ptr, ny, nx);
        verify_panel(player_ptr);
        if (mpe_mode & MPE_FORGET_FLOW) {
            forget_flow(floor_ptr);
            player_ptr->update |= PU_UN_VIEW;
            player_ptr->redraw |= PR_MAP;
        }

        player_ptr->update |= PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_DISTANCE;
        player_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
        if ((!player_ptr->blind && !no_lite(player_ptr)) || !is_trap(player_ptr, g_ptr->feat))
            g_ptr->info &= ~(CAVE_UNSAFE);

        if (floor_ptr->dun_level && d_info[player_ptr->dungeon_idx].flags.has(DF::FORGET))
            wiz_dark(player_ptr);

        if (mpe_mode & MPE_HANDLE_STUFF)
            handle_stuff(player_ptr);

        if (player_ptr->pclass == CLASS_NINJA) {
            if (g_ptr->info & (CAVE_GLOW))
                set_superstealth(player_ptr, false);
            else if (player_ptr->cur_lite <= 0)
                set_superstealth(player_ptr, true);
        }

        if ((player_ptr->action == ACTION_HAYAGAKE)
            && (f_ptr->flags.has_not(FF::PROJECT) || (!player_ptr->levitation && f_ptr->flags.has(FF::DEEP)))) {
            msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
            set_action(player_ptr, ACTION_NONE);
        }

        if (player_ptr->prace == player_race_type::MERFOLK) {
            if (f_ptr->flags.has(FF::WATER) ^ of_ptr->flags.has(FF::WATER)) {
                player_ptr->update |= PU_BONUS;
                update_creature(player_ptr);
            }
        }
    }

    if (mpe_mode & MPE_ENERGY_USE) {
        if (music_singing(player_ptr, MUSIC_WALL)) {
            (void)project(player_ptr, 0, 0, player_ptr->y, player_ptr->x, (60 + player_ptr->lev), GF_DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM);
            if (!player_bold(player_ptr, ny, nx) || player_ptr->is_dead || player_ptr->leaving)
                return false;
        }

        if ((player_ptr->skill_fos >= 50) || (0 == randint0(50 - player_ptr->skill_fos)))
            search(player_ptr);

        if (player_ptr->action == ACTION_SEARCH)
            search(player_ptr);
    }

    if (!(mpe_mode & MPE_DONT_PICKUP))
        carry(player_ptr, (mpe_mode & MPE_DO_PICKUP) ? true : false);

    if (!player_ptr->running) {
        // 自動拾い/自動破壊により床上のアイテムリストが変化した可能性があるので表示を更新
        set_bits(player_ptr->window_flags, PW_FLOOR_ITEM_LIST);
        window_stuff(player_ptr);
    }

    PlayerEnergy energy(player_ptr);
    if (f_ptr->flags.has(FF::STORE)) {
        disturb(player_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_STORE;
    } else if (f_ptr->flags.has(FF::BLDG)) {
        disturb(player_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_BUILDING;
    } else if (f_ptr->flags.has(FF::QUEST_ENTER)) {
        disturb(player_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_QUEST;
    } else if (f_ptr->flags.has(FF::QUEST_EXIT)) {
        if (quest[floor_ptr->inside_quest].type == QUEST_TYPE_FIND_EXIT)
            complete_quest(player_ptr, floor_ptr->inside_quest);

        leave_quest_check(player_ptr);
        floor_ptr->inside_quest = g_ptr->special;
        floor_ptr->dun_level = 0;
        if (!floor_ptr->inside_quest)
            player_ptr->word_recall = 0;
        player_ptr->oldpx = 0;
        player_ptr->oldpy = 0;
        player_ptr->leaving = true;
    } else if (f_ptr->flags.has(FF::HIT_TRAP) && !(mpe_mode & MPE_STAYING)) {
        disturb(player_ptr, false, true);
        if (g_ptr->mimic || f_ptr->flags.has(FF::SECRET)) {
            msg_print(_("トラップだ！", "You found a trap!"));
            disclose_grid(player_ptr, player_ptr->y, player_ptr->x);
        }

        hit_trap(player_ptr, (mpe_mode & MPE_BREAK_TRAP) ? true : false);
        if (!player_bold(player_ptr, ny, nx) || player_ptr->is_dead || player_ptr->leaving)
            return false;
    }

    if (!(mpe_mode & MPE_STAYING) && (disturb_trap_detect || alert_trap_detect) && player_ptr->dtrap && !(g_ptr->info & CAVE_IN_DETECT)) {
        player_ptr->dtrap = false;
        if (!(g_ptr->info & CAVE_UNSAFE)) {
            if (alert_trap_detect)
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));

            if (disturb_trap_detect)
                disturb(player_ptr, false, true);
        }
    }

    return player_bold(player_ptr, ny, nx) && !player_ptr->is_dead && !player_ptr->leaving;
}

/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat 地形ID
 * @return トラップが自動的に無効ならばTRUEを返す
 */
bool trap_can_be_ignored(player_type *player_ptr, FEAT_IDX feat)
{
    feature_type *f_ptr = &f_info[feat];
    if (f_ptr->flags.has_not(FF::TRAP))
        return true;

    switch (f_ptr->subtype) {
    case TRAP_TRAPDOOR:
    case TRAP_PIT:
    case TRAP_SPIKED_PIT:
    case TRAP_POISON_PIT:
        if (player_ptr->levitation)
            return true;
        break;
    case TRAP_TELEPORT:
        if (player_ptr->anti_tele)
            return true;
        break;
    case TRAP_FIRE:
        if (has_immune_fire(player_ptr))
            return true;
        break;
    case TRAP_ACID:
        if (has_immune_acid(player_ptr))
            return true;
        break;
    case TRAP_BLIND:
        if (has_resist_blind(player_ptr))
            return true;
        break;
    case TRAP_CONFUSE:
        if (has_resist_conf(player_ptr))
            return true;
        break;
    case TRAP_POISON:
        if (has_resist_pois(player_ptr))
            return true;
        break;
    case TRAP_SLEEP:
        if (player_ptr->free_act)
            return true;
        break;
    }

    return false;
}
