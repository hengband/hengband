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
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 対象となるマスのY座標
 * @param x 対象となるマスのX座標
 */
static void discover_hidden_things(player_type *creature_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->mimic && is_trap(creature_ptr, g_ptr->feat)) {
        disclose_grid(creature_ptr, y, x);
        msg_print(_("トラップを発見した。", "You have found a trap."));
        disturb(creature_ptr, false, true);
    }

    if (is_hidden_door(creature_ptr, g_ptr)) {
        msg_print(_("隠しドアを発見した。", "You have found a secret door."));
        disclose_grid(creature_ptr, y, x);
        disturb(creature_ptr, false, false);
    }

    for (const auto this_o_idx : g_ptr->o_idx_list) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (o_ptr->tval != TV_CHEST)
            continue;
        if (!chest_traps[o_ptr->pval])
            continue;
        if (!object_is_known(o_ptr)) {
            msg_print(_("箱に仕掛けられたトラップを発見した！", "You have discovered a trap on the chest!"));
            object_known(o_ptr);
            disturb(creature_ptr, false, false);
        }
    }
}

/*!
 * @brief プレイヤーの探索処理判定
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void search(player_type *creature_ptr)
{
    PERCENTAGE chance = creature_ptr->skill_srh;
    if (creature_ptr->blind || no_lite(creature_ptr))
        chance = chance / 10;

    if (creature_ptr->confused || creature_ptr->image)
        chance = chance / 10;

    for (DIRECTION i = 0; i < 9; ++i)
        if (randint0(100) < chance)
            discover_hidden_things(creature_ptr, creature_ptr->y + ddy_ddd[i], creature_ptr->x + ddx_ddd[i]);
}

/*!
 * @brief 移動に伴うプレイヤーのステータス変化処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param ny 移動先Y座標
 * @param nx 移動先X座標
 * @param mpe_mode 移動オプションフラグ
 * @return プレイヤーが死亡やフロア離脱を行わず、実際に移動が可能ならばTRUEを返す。
 */
bool move_player_effect(player_type *creature_ptr, POSITION ny, POSITION nx, BIT_FLAGS mpe_mode)
{
    POSITION oy = creature_ptr->y;
    POSITION ox = creature_ptr->x;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[ny][nx];
    grid_type *oc_ptr = &floor_ptr->grid_array[oy][ox];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    feature_type *of_ptr = &f_info[oc_ptr->feat];

    if (!(mpe_mode & MPE_STAYING)) {
        MONSTER_IDX om_idx = oc_ptr->m_idx;
        MONSTER_IDX nm_idx = g_ptr->m_idx;
        creature_ptr->y = ny;
        creature_ptr->x = nx;
        if (!(mpe_mode & MPE_DONT_SWAP_MON)) {
            g_ptr->m_idx = om_idx;
            oc_ptr->m_idx = nm_idx;
            if (om_idx > 0) {
                monster_type *om_ptr = &floor_ptr->m_list[om_idx];
                om_ptr->fy = ny;
                om_ptr->fx = nx;
                update_monster(creature_ptr, om_idx, true);
            }

            if (nm_idx > 0) {
                monster_type *nm_ptr = &floor_ptr->m_list[nm_idx];
                nm_ptr->fy = oy;
                nm_ptr->fx = ox;
                update_monster(creature_ptr, nm_idx, true);
            }
        }

        lite_spot(creature_ptr, oy, ox);
        lite_spot(creature_ptr, ny, nx);
        verify_panel(creature_ptr);
        if (mpe_mode & MPE_FORGET_FLOW) {
            forget_flow(floor_ptr);
            creature_ptr->update |= PU_UN_VIEW;
            creature_ptr->redraw |= PR_MAP;
        }

        creature_ptr->update |= PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_DISTANCE;
        creature_ptr->window_flags |= PW_OVERHEAD | PW_DUNGEON;
        if ((!creature_ptr->blind && !no_lite(creature_ptr)) || !is_trap(creature_ptr, g_ptr->feat))
            g_ptr->info &= ~(CAVE_UNSAFE);

        if (floor_ptr->dun_level && d_info[creature_ptr->dungeon_idx].flags.has(DF::FORGET))
            wiz_dark(creature_ptr);

        if (mpe_mode & MPE_HANDLE_STUFF)
            handle_stuff(creature_ptr);

        if (creature_ptr->pclass == CLASS_NINJA) {
            if (g_ptr->info & (CAVE_GLOW))
                set_superstealth(creature_ptr, false);
            else if (creature_ptr->cur_lite <= 0)
                set_superstealth(creature_ptr, true);
        }

        if ((creature_ptr->action == ACTION_HAYAGAKE)
            && (f_ptr->flags.has_not(FF::PROJECT) || (!creature_ptr->levitation && f_ptr->flags.has(FF::DEEP)))) {
            msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
            set_action(creature_ptr, ACTION_NONE);
        }

        if (creature_ptr->prace == player_race_type::MERFOLK) {
            if (f_ptr->flags.has(FF::WATER) ^ of_ptr->flags.has(FF::WATER)) {
                creature_ptr->update |= PU_BONUS;
                update_creature(creature_ptr);
            }
        }
    }

    if (mpe_mode & MPE_ENERGY_USE) {
        if (music_singing(creature_ptr, MUSIC_WALL)) {
            (void)project(creature_ptr, 0, 0, creature_ptr->y, creature_ptr->x, (60 + creature_ptr->lev), GF_DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM);
            if (!player_bold(creature_ptr, ny, nx) || creature_ptr->is_dead || creature_ptr->leaving)
                return false;
        }

        if ((creature_ptr->skill_fos >= 50) || (0 == randint0(50 - creature_ptr->skill_fos)))
            search(creature_ptr);

        if (creature_ptr->action == ACTION_SEARCH)
            search(creature_ptr);
    }

    if (!(mpe_mode & MPE_DONT_PICKUP))
        carry(creature_ptr, (mpe_mode & MPE_DO_PICKUP) ? true : false);

    if (!creature_ptr->running) {
        // 自動拾い/自動破壊により床上のアイテムリストが変化した可能性があるので表示を更新
        set_bits(creature_ptr->window_flags, PW_FLOOR_ITEM_LIST);
        window_stuff(creature_ptr);
    }

    PlayerEnergy energy(creature_ptr);
    if (f_ptr->flags.has(FF::STORE)) {
        disturb(creature_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_STORE;
    } else if (f_ptr->flags.has(FF::BLDG)) {
        disturb(creature_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_BUILDING;
    } else if (f_ptr->flags.has(FF::QUEST_ENTER)) {
        disturb(creature_ptr, false, true);
        energy.reset_player_turn();
        command_new = SPECIAL_KEY_QUEST;
    } else if (f_ptr->flags.has(FF::QUEST_EXIT)) {
        if (quest[floor_ptr->inside_quest].type == QUEST_TYPE_FIND_EXIT)
            complete_quest(creature_ptr, floor_ptr->inside_quest);

        leave_quest_check(creature_ptr);
        floor_ptr->inside_quest = g_ptr->special;
        floor_ptr->dun_level = 0;
        if (!floor_ptr->inside_quest)
            creature_ptr->word_recall = 0;
        creature_ptr->oldpx = 0;
        creature_ptr->oldpy = 0;
        creature_ptr->leaving = true;
    } else if (f_ptr->flags.has(FF::HIT_TRAP) && !(mpe_mode & MPE_STAYING)) {
        disturb(creature_ptr, false, true);
        if (g_ptr->mimic || f_ptr->flags.has(FF::SECRET)) {
            msg_print(_("トラップだ！", "You found a trap!"));
            disclose_grid(creature_ptr, creature_ptr->y, creature_ptr->x);
        }

        hit_trap(creature_ptr, (mpe_mode & MPE_BREAK_TRAP) ? true : false);
        if (!player_bold(creature_ptr, ny, nx) || creature_ptr->is_dead || creature_ptr->leaving)
            return false;
    }

    if (!(mpe_mode & MPE_STAYING) && (disturb_trap_detect || alert_trap_detect) && creature_ptr->dtrap && !(g_ptr->info & CAVE_IN_DETECT)) {
        creature_ptr->dtrap = false;
        if (!(g_ptr->info & CAVE_UNSAFE)) {
            if (alert_trap_detect)
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));

            if (disturb_trap_detect)
                disturb(creature_ptr, false, true);
        }
    }

    return player_bold(creature_ptr, ny, nx) && !creature_ptr->is_dead && !creature_ptr->leaving;
}

/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param feat 地形ID
 * @return トラップが自動的に無効ならばTRUEを返す
 */
bool trap_can_be_ignored(player_type *creature_ptr, FEAT_IDX feat)
{
    feature_type *f_ptr = &f_info[feat];
    if (f_ptr->flags.has_not(FF::TRAP))
        return true;

    switch (f_ptr->subtype) {
    case TRAP_TRAPDOOR:
    case TRAP_PIT:
    case TRAP_SPIKED_PIT:
    case TRAP_POISON_PIT:
        if (creature_ptr->levitation)
            return true;
        break;
    case TRAP_TELEPORT:
        if (creature_ptr->anti_tele)
            return true;
        break;
    case TRAP_FIRE:
        if (has_immune_fire(creature_ptr))
            return true;
        break;
    case TRAP_ACID:
        if (has_immune_acid(creature_ptr))
            return true;
        break;
    case TRAP_BLIND:
        if (has_resist_blind(creature_ptr))
            return true;
        break;
    case TRAP_CONFUSE:
        if (has_resist_conf(creature_ptr))
            return true;
        break;
    case TRAP_POISON:
        if (has_resist_pois(creature_ptr))
            return true;
        break;
    case TRAP_SLEEP:
        if (creature_ptr->free_act)
            return true;
        break;
    }

    return false;
}
