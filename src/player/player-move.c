/*!
 *  @brief プレイヤーの移動処理 / Movement commands
 *  @date 2014/01/02
 *  @author
 *  Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 */

#include "player/player-move.h"
#include "action/open-close-execution.h"
#include "action/travel-execution.h"
#include "action/run-execution.h"
#include "art-definition/art-bow-types.h"
#include "art-definition/art-sword-types.h"
#include "autopick/autopick.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-others.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "flavor/flavor-describer.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/player-inventory.h"
#include "io/input-key-requester.h"
#include "io/targeting.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/special-object-flags.h"
#include "object/item-tester-hooker.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "object/warning.h"
#include "perception/object-perception.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
#include "player/player-status.h"
#include "realm/realm-song-numbers.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-perception.h"
#include "spell/process-effect.h"
#include "spell/spell-types.h"
#include "status/action-setter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 地形やその上のアイテムの隠された要素を全て明かす /
 * Search for hidden things
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 対象となるマスのY座標
 * @param x 対象となるマスのX座標
 * @return なし
 */
static void discover_hidden_things(player_type *creature_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->mimic && is_trap(creature_ptr, g_ptr->feat)) {
        disclose_grid(creature_ptr, y, x);
        msg_print(_("トラップを発見した。", "You have found a trap."));
        disturb(creature_ptr, FALSE, TRUE);
    }

    if (is_hidden_door(creature_ptr, g_ptr)) {
        msg_print(_("隠しドアを発見した。", "You have found a secret door."));
        disclose_grid(creature_ptr, y, x);
        disturb(creature_ptr, FALSE, FALSE);
    }

    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (o_ptr->tval != TV_CHEST)
            continue;
        if (!chest_traps[o_ptr->pval])
            continue;
        if (!object_is_known(o_ptr)) {
            msg_print(_("箱に仕掛けられたトラップを発見した！", "You have discovered a trap on the chest!"));
            object_known(o_ptr);
            disturb(creature_ptr, FALSE, FALSE);
        }
    }
}

/*!
 * @brief プレイヤーの探索処理判定
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
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
 * @brief パターンによる移動制限処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param c_y プレイヤーの移動元Y座標
 * @param c_x プレイヤーの移動元X座標
 * @param n_y プレイヤーの移動先Y座標
 * @param n_x プレイヤーの移動先X座標
 * @return 移動処理が可能である場合（可能な場合に選択した場合）TRUEを返す。
 */
bool pattern_seq(player_type *creature_ptr, POSITION c_y, POSITION c_x, POSITION n_y, POSITION n_x)
{
    feature_type *cur_f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[c_y][c_x].feat];
    feature_type *new_f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[n_y][n_x].feat];
    bool is_pattern_tile_cur = have_flag(cur_f_ptr->flags, FF_PATTERN);
    bool is_pattern_tile_new = have_flag(new_f_ptr->flags, FF_PATTERN);
    if (!is_pattern_tile_cur && !is_pattern_tile_new)
        return TRUE;

    int pattern_type_cur = is_pattern_tile_cur ? cur_f_ptr->subtype : NOT_PATTERN_TILE;
    int pattern_type_new = is_pattern_tile_new ? new_f_ptr->subtype : NOT_PATTERN_TILE;
    if (pattern_type_new == PATTERN_TILE_START) {
        if (!is_pattern_tile_cur && !creature_ptr->confused && !creature_ptr->stun && !creature_ptr->image) {
            if (get_check(_("パターンの上を歩き始めると、全てを歩かなければなりません。いいですか？",
                    "If you start walking the Pattern, you must walk the whole way. Ok? ")))
                return TRUE;
            else
                return FALSE;
        } else
            return TRUE;
    }

    if ((pattern_type_new == PATTERN_TILE_OLD) || (pattern_type_new == PATTERN_TILE_END) || (pattern_type_new == PATTERN_TILE_WRECKED)) {
        if (is_pattern_tile_cur) {
            return TRUE;
        } else {
            msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。", "You must start walking the Pattern from the startpoint."));
            return FALSE;
        }
    }

    if ((pattern_type_new == PATTERN_TILE_TELEPORT) || (pattern_type_cur == PATTERN_TILE_TELEPORT))
        return TRUE;

    if (pattern_type_cur == PATTERN_TILE_START) {
        if (is_pattern_tile_new)
            return TRUE;
        else {
            msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));
            return FALSE;
        }
    }

    if ((pattern_type_cur == PATTERN_TILE_OLD) || (pattern_type_cur == PATTERN_TILE_END) || (pattern_type_cur == PATTERN_TILE_WRECKED)) {
        if (!is_pattern_tile_new) {
            msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
            return FALSE;
        } else {
            return TRUE;
        }
    }

    if (!is_pattern_tile_cur) {
        msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。", "You must start walking the Pattern from the startpoint."));

        return FALSE;
    }

    byte ok_move = PATTERN_TILE_START;
    switch (pattern_type_cur) {
    case PATTERN_TILE_1:
        ok_move = PATTERN_TILE_2;
        break;
    case PATTERN_TILE_2:
        ok_move = PATTERN_TILE_3;
        break;
    case PATTERN_TILE_3:
        ok_move = PATTERN_TILE_4;
        break;
    case PATTERN_TILE_4:
        ok_move = PATTERN_TILE_1;
        break;
    default:
        if (current_world_ptr->wizard)
            msg_format(_("おかしなパターン歩行、%d。", "Funny Pattern walking, %d."), pattern_type_cur);
        return TRUE;
    }

    if ((pattern_type_new == ok_move) || (pattern_type_new == pattern_type_cur))
        return TRUE;

    if (!is_pattern_tile_new)
        msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
    else
        msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));

    return FALSE;
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
                update_monster(creature_ptr, om_idx, TRUE);
            }

            if (nm_idx > 0) {
                monster_type *nm_ptr = &floor_ptr->m_list[nm_idx];
                nm_ptr->fy = oy;
                nm_ptr->fx = ox;
                update_monster(creature_ptr, nm_idx, TRUE);
            }
        }

        lite_spot(creature_ptr, oy, ox);
        lite_spot(creature_ptr, ny, nx);
        verify_panel(creature_ptr);
        if (mpe_mode & MPE_FORGET_FLOW) {
            forget_flow(floor_ptr);
            creature_ptr->update |= (PU_UN_VIEW);
            creature_ptr->redraw |= (PR_MAP);
        }

        creature_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_DISTANCE);
        creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        if ((!creature_ptr->blind && !no_lite(creature_ptr)) || !is_trap(creature_ptr, g_ptr->feat))
            g_ptr->info &= ~(CAVE_UNSAFE);

        if (floor_ptr->dun_level && (d_info[creature_ptr->dungeon_idx].flags1 & DF1_FORGET))
            wiz_dark(creature_ptr);

        if (mpe_mode & MPE_HANDLE_STUFF)
            handle_stuff(creature_ptr);

        if (creature_ptr->pclass == CLASS_NINJA) {
            if (g_ptr->info & (CAVE_GLOW))
                set_superstealth(creature_ptr, FALSE);
            else if (creature_ptr->cur_lite <= 0)
                set_superstealth(creature_ptr, TRUE);
        }

        if ((creature_ptr->action == ACTION_HAYAGAKE)
            && (!have_flag(f_ptr->flags, FF_PROJECT) || (!creature_ptr->levitation && have_flag(f_ptr->flags, FF_DEEP)))) {
            msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
            set_action(creature_ptr, ACTION_NONE);
        }

        if (creature_ptr->prace == RACE_MERFOLK) {
            if (have_flag(f_ptr->flags, FF_WATER) ^ have_flag(of_ptr->flags, FF_WATER)) {
                creature_ptr->update |= PU_BONUS;
                update_creature(creature_ptr);
            }
        }
    }

    if (mpe_mode & MPE_ENERGY_USE) {
        if (music_singing(creature_ptr, MUSIC_WALL)) {
            (void)project(creature_ptr, 0, 0, creature_ptr->y, creature_ptr->x, (60 + creature_ptr->lev), GF_DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM, -1);
            if (!player_bold(creature_ptr, ny, nx) || creature_ptr->is_dead || creature_ptr->leaving)
                return FALSE;
        }

        if ((creature_ptr->skill_fos >= 50) || (0 == randint0(50 - creature_ptr->skill_fos)))
            search(creature_ptr);

        if (creature_ptr->action == ACTION_SEARCH)
            search(creature_ptr);
    }

    if (!(mpe_mode & MPE_DONT_PICKUP))
        carry(creature_ptr, (mpe_mode & MPE_DO_PICKUP) ? TRUE : FALSE);

    if (have_flag(f_ptr->flags, FF_STORE)) {
        disturb(creature_ptr, FALSE, TRUE);
        free_turn(creature_ptr);
        command_new = SPECIAL_KEY_STORE;
    } else if (have_flag(f_ptr->flags, FF_BLDG)) {
        disturb(creature_ptr, FALSE, TRUE);
        free_turn(creature_ptr);
        command_new = SPECIAL_KEY_BUILDING;
    } else if (have_flag(f_ptr->flags, FF_QUEST_ENTER)) {
        disturb(creature_ptr, FALSE, TRUE);
        free_turn(creature_ptr);
        command_new = SPECIAL_KEY_QUEST;
    } else if (have_flag(f_ptr->flags, FF_QUEST_EXIT)) {
        if (quest[floor_ptr->inside_quest].type == QUEST_TYPE_FIND_EXIT)
            complete_quest(creature_ptr, floor_ptr->inside_quest);

        leave_quest_check(creature_ptr);
        floor_ptr->inside_quest = g_ptr->special;
        floor_ptr->dun_level = 0;
        creature_ptr->oldpx = 0;
        creature_ptr->oldpy = 0;
        creature_ptr->leaving = TRUE;
    } else if (have_flag(f_ptr->flags, FF_HIT_TRAP) && !(mpe_mode & MPE_STAYING)) {
        disturb(creature_ptr, FALSE, TRUE);
        if (g_ptr->mimic || have_flag(f_ptr->flags, FF_SECRET)) {
            msg_print(_("トラップだ！", "You found a trap!"));
            disclose_grid(creature_ptr, creature_ptr->y, creature_ptr->x);
        }

        hit_trap(creature_ptr, (mpe_mode & MPE_BREAK_TRAP) ? TRUE : FALSE);
        if (!player_bold(creature_ptr, ny, nx) || creature_ptr->is_dead || creature_ptr->leaving)
            return FALSE;
    }

    if (!(mpe_mode & MPE_STAYING) && (disturb_trap_detect || alert_trap_detect) && creature_ptr->dtrap && !(g_ptr->info & CAVE_IN_DETECT)) {
        creature_ptr->dtrap = FALSE;
        if (!(g_ptr->info & CAVE_UNSAFE)) {
            if (alert_trap_detect)
                msg_print(_("* 注意:この先はトラップの感知範囲外です！ *", "*Leaving trap detect region!*"));

            if (disturb_trap_detect)
                disturb(creature_ptr, FALSE, TRUE);
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
    if (!have_flag(f_ptr->flags, FF_TRAP))
        return TRUE;

    switch (f_ptr->subtype) {
    case TRAP_TRAPDOOR:
    case TRAP_PIT:
    case TRAP_SPIKED_PIT:
    case TRAP_POISON_PIT:
        if (creature_ptr->levitation)
            return TRUE;
        break;
    case TRAP_TELEPORT:
        if (creature_ptr->anti_tele)
            return TRUE;
        break;
    case TRAP_FIRE:
        if (creature_ptr->immune_fire)
            return TRUE;
        break;
    case TRAP_ACID:
        if (creature_ptr->immune_acid)
            return TRUE;
        break;
    case TRAP_BLIND:
        if (creature_ptr->resist_blind)
            return TRUE;
        break;
    case TRAP_CONFUSE:
        if (creature_ptr->resist_conf)
            return TRUE;
        break;
    case TRAP_POISON:
        if (creature_ptr->resist_pois)
            return TRUE;
        break;
    case TRAP_SLEEP:
        if (creature_ptr->free_act)
            return TRUE;
        break;
    }

    return FALSE;
}

#define TRAVEL_UNABLE 9999

static int flow_head = 0;
static int flow_tail = 0;
static POSITION temp2_x[MAX_SHORT];
static POSITION temp2_y[MAX_SHORT];

/*!
 * @brief トラベル処理の記憶配列を初期化する Hack: forget the "flow" information
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @return なし
 */
void forget_travel_flow(floor_type *floor_ptr)
{
    for (POSITION y = 0; y < floor_ptr->height; y++)
        for (POSITION x = 0; x < floor_ptr->width; x++)
            travel.cost[y][x] = MAX_SHORT;

    travel.y = travel.x = 0;
}

/*!
 * @brief トラベル処理中に地形に応じた移動コスト基準を返す
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param y 該当地点のY座標
 * @param x 該当地点のX座標
 * @return コスト値
 */
static int travel_flow_cost(player_type *creature_ptr, POSITION y, POSITION x)
{
    int cost = 1;
    feature_type *f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[y][x].feat];
    if (have_flag(f_ptr->flags, FF_AVOID_RUN))
        cost += 1;

    if (have_flag(f_ptr->flags, FF_WATER) && have_flag(f_ptr->flags, FF_DEEP) && !creature_ptr->levitation)
        cost += 5;

    if (have_flag(f_ptr->flags, FF_LAVA)) {
        int lava = 2;
        if (!creature_ptr->resist_fire)
            lava *= 2;

        if (!creature_ptr->levitation)
            lava *= 2;

        if (have_flag(f_ptr->flags, FF_DEEP))
            lava *= 2;

        cost += lava;
    }

    if (creature_ptr->current_floor_ptr->grid_array[y][x].info & (CAVE_MARK)) {
        if (have_flag(f_ptr->flags, FF_DOOR))
            cost += 1;

        if (have_flag(f_ptr->flags, FF_TRAP))
            cost += 10;
    }

    return cost;
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のサブルーチン
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param n 現在のコスト
 * @param wall プレイヤーが壁の中にいるならばTRUE
 * @return なし
 */
static void travel_flow_aux(player_type *creature_ptr, POSITION y, POSITION x, int n, bool wall)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    if (!in_bounds(floor_ptr, y, x))
        return;

    if (floor_ptr->dun_level > 0 && !(g_ptr->info & CAVE_KNOWN))
        return;

    int add_cost = 1;
    int from_wall = (n / TRAVEL_UNABLE);
    if (have_flag(f_ptr->flags, FF_WALL) || have_flag(f_ptr->flags, FF_CAN_DIG) || (have_flag(f_ptr->flags, FF_DOOR) && floor_ptr->grid_array[y][x].mimic)
        || (!have_flag(f_ptr->flags, FF_MOVE) && have_flag(f_ptr->flags, FF_CAN_FLY) && !creature_ptr->levitation)) {
        if (!wall || !from_wall)
            return;

        add_cost += TRAVEL_UNABLE;
    } else
        add_cost = travel_flow_cost(creature_ptr, y, x);

    int base_cost = (n % TRAVEL_UNABLE);
    int cost = base_cost + add_cost;
    if (travel.cost[y][x] <= cost)
        return;

    travel.cost[y][x] = cost;
    int old_head = flow_head;
    temp2_y[flow_head] = y;
    temp2_x[flow_head] = x;
    if (++flow_head == MAX_SHORT)
        flow_head = 0;

    if (flow_head == flow_tail)
        flow_head = old_head;
}

/*!
 * @brief トラベル処理の到達地点までの行程を得る処理のメインルーチン
 * @param creature_ptr	プレーヤーへの参照ポインタ
 * @param ty 目標地点のY座標
 * @param tx 目標地点のX座標
 * @return なし
 */
static void travel_flow(player_type *creature_ptr, POSITION ty, POSITION tx)
{
    flow_head = flow_tail = 0;
    bool wall = FALSE;
    feature_type *f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat];
    if (!have_flag(f_ptr->flags, FF_MOVE))
        wall = TRUE;

    travel_flow_aux(creature_ptr, ty, tx, 0, wall);
    POSITION x, y;
    while (flow_head != flow_tail) {
        y = temp2_y[flow_tail];
        x = temp2_x[flow_tail];
        if (++flow_tail == MAX_SHORT)
            flow_tail = 0;

        for (DIRECTION d = 0; d < 8; d++)
            travel_flow_aux(creature_ptr, y + ddy_ddd[d], x + ddx_ddd[d], travel.cost[y][x], wall);
    }

    flow_head = flow_tail = 0;
}

/*!
 * @brief トラベル処理のメインルーチン
 * @return なし
 */
void do_cmd_travel(player_type *creature_ptr)
{
    POSITION x, y;
    if (travel.x != 0 && travel.y != 0 && get_check(_("トラベルを継続しますか？", "Do you continue to travel?"))) {
        y = travel.y;
        x = travel.x;
    } else if (!tgt_pt(creature_ptr, &x, &y))
        return;

    if ((x == creature_ptr->x) && (y == creature_ptr->y)) {
        msg_print(_("すでにそこにいます！", "You are already there!!"));
        return;
    }

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    feature_type *f_ptr;
    f_ptr = &f_info[floor_ptr->grid_array[y][x].feat];
    if ((floor_ptr->grid_array[y][x].info & CAVE_MARK)
        && (have_flag(f_ptr->flags, FF_WALL) || have_flag(f_ptr->flags, FF_CAN_DIG)
            || (have_flag(f_ptr->flags, FF_DOOR) && floor_ptr->grid_array[y][x].mimic))) {
        msg_print(_("そこには行くことができません！", "You cannot travel there!"));
        return;
    }

    forget_travel_flow(creature_ptr->current_floor_ptr);
    travel_flow(creature_ptr, y, x);
    travel.x = x;
    travel.y = y;
    travel.run = 255;
    travel.dir = 0;
    POSITION dx = abs(creature_ptr->x - x);
    POSITION dy = abs(creature_ptr->y - y);
    POSITION sx = ((x == creature_ptr->x) || (dx < dy)) ? 0 : ((x > creature_ptr->x) ? 1 : -1);
    POSITION sy = ((y == creature_ptr->y) || (dy < dx)) ? 0 : ((y > creature_ptr->y) ? 1 : -1);
    for (int i = 1; i <= 9; i++)
        if ((sx == ddx[i]) && (sy == ddy[i]))
            travel.dir = i;
}
