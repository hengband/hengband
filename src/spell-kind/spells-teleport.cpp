/*!
 * @brief テレポート魔法全般
 * @date 2020/06/04
 * @author Hourier
 */

#include "spell-kind/spells-teleport.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/target-checker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief モンスターとの位置交換処理 / Switch position with a monster.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool teleport_swap(player_type *player_ptr, DIRECTION dir)
{
    POSITION tx, ty;
    if ((dir == 5) && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;
    } else {
        tx = player_ptr->x + ddx[dir];
        ty = player_ptr->y + ddy[dir];
    }

    if (player_ptr->anti_tele) {
        msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
        return false;
    }

    grid_type *g_ptr;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[ty][tx];
    if (!g_ptr->m_idx || (g_ptr->m_idx == player_ptr->riding)) {
        msg_print(_("それとは場所を交換できません。", "You can't trade places with that!"));
        return false;
    }

    if ((g_ptr->is_icky()) || (distance(ty, tx, player_ptr->y, player_ptr->x) > player_ptr->lev * 3 / 2 + 10)) {
        msg_print(_("失敗した。", "Failed to swap."));
        return false;
    }

    monster_type *m_ptr;
    monster_race *r_ptr;
    m_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
    r_ptr = &r_info[m_ptr->r_idx];

    (void)set_monster_csleep(player_ptr, g_ptr->m_idx, 0);

    if (r_ptr->flagsr & RFR_RES_TELE) {
        msg_print(_("テレポートを邪魔された！", "Your teleportation is blocked!"));
        if (is_original_ap_and_seen(player_ptr, m_ptr))
            r_ptr->r_flagsr |= RFR_RES_TELE;
        return false;
    }

    sound(SOUND_TELEPORT);
    (void)move_player_effect(player_ptr, ty, tx, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
    return true;
}

/*!
 * @brief モンスター用テレポート処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param distance 移動距離
 * @return 作用が実際にあった場合TRUEを返す
 */
bool teleport_monster(player_type *player_ptr, DIRECTION dir, int distance)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_KILL;
    return (project_hook(player_ptr, GF_AWAY_ALL, dir, distance, flg));
}

/*!
 * @brief モンスターのテレポートアウェイ処理 /
 * Teleport a monster, normally up to "dis" grids away.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param dis テレポート距離
 * @param mode オプション
 * @return テレポートが実際に行われたらtrue
 * @details
 * Attempt to move the monster at least "dis/2" grids away.
 * But allow variation to prevent infinite loops.
 */
bool teleport_away(player_type *player_ptr, MONSTER_IDX m_idx, POSITION dis, teleport_flags mode)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    if (!monster_is_valid(m_ptr))
        return false;

    if ((mode & TELEPORT_DEC_VALOUR) && (((player_ptr->chp * 10) / player_ptr->mhp) > 5) && (4 + randint1(5) < ((player_ptr->chp * 10) / player_ptr->mhp))) {
        chg_virtue(player_ptr, V_VALOUR, -1);
    }

    POSITION oy = m_ptr->fy;
    POSITION ox = m_ptr->fx;
    POSITION min = dis / 2;
    int tries = 0;
    POSITION ny = 0, nx = 0;
    bool look = true;
    while (look) {
        tries++;
        if (dis > 200)
            dis = 200;

        for (int i = 0; i < 500; i++) {
            while (true) {
                ny = rand_spread(oy, dis);
                nx = rand_spread(ox, dis);
                POSITION d = distance(oy, ox, ny, nx);
                if ((d >= min) && (d <= dis))
                    break;
            }

            if (!in_bounds(player_ptr->current_floor_ptr, ny, nx))
                continue;
            if (!cave_monster_teleportable_bold(player_ptr, m_idx, ny, nx, mode))
                continue;
            if (!(player_ptr->current_floor_ptr->inside_quest || player_ptr->current_floor_ptr->inside_arena))
                if (player_ptr->current_floor_ptr->grid_array[ny][nx].is_icky())
                    continue;

            look = false;
            break;
        }

        dis = dis * 2;
        min = min / 2;
        const int MAX_TELEPORT_TRIES = 100;
        if (tries > MAX_TELEPORT_TRIES)
            return false;
    }

    sound(SOUND_TPOTHER);
    player_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;
    player_ptr->current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;

    m_ptr->fy = ny;
    m_ptr->fx = nx;

    reset_target(m_ptr);
    update_monster(player_ptr, m_idx, true);
    lite_spot(player_ptr, oy, ox);
    lite_spot(player_ptr, ny, nx);

    if (r_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
        player_ptr->update |= (PU_MON_LITE);

    return true;
}

/*!
 * @brief モンスターを指定された座標付近にテレポートする /
 * Teleport monster next to a grid near the given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param ty 目安Y座標
 * @param tx 目安X座標
 * @param power テレポート成功確率
 * @param mode オプション
 */
void teleport_monster_to(player_type *player_ptr, MONSTER_IDX m_idx, POSITION ty, POSITION tx, int power, teleport_flags mode)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    if (!m_ptr->r_idx)
        return;
    if (randint1(100) > power)
        return;

    POSITION ny = m_ptr->fy;
    POSITION nx = m_ptr->fx;
    POSITION oy = m_ptr->fy;
    POSITION ox = m_ptr->fx;

    POSITION dis = 2;
    int min = dis / 2;
    int attempts = 500;
    bool look = true;
    while (look && --attempts) {
        if (dis > 200)
            dis = 200;

        for (int i = 0; i < 500; i++) {
            while (true) {
                ny = rand_spread(ty, dis);
                nx = rand_spread(tx, dis);
                int d = distance(ty, tx, ny, nx);
                if ((d >= min) && (d <= dis))
                    break;
            }

            if (!in_bounds(player_ptr->current_floor_ptr, ny, nx))
                continue;
            if (!cave_monster_teleportable_bold(player_ptr, m_idx, ny, nx, mode))
                continue;

            look = false;
            break;
        }

        dis = dis * 2;
        min = min / 2;
    }

    if (attempts < 1)
        return;

    sound(SOUND_TPOTHER);
    player_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;
    player_ptr->current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;

    m_ptr->fy = ny;
    m_ptr->fx = nx;

    update_monster(player_ptr, m_idx, true);
    lite_spot(player_ptr, oy, ox);
    lite_spot(player_ptr, ny, nx);

    if (r_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
        player_ptr->update |= (PU_MON_LITE);
}

/*!
 * @brief プレイヤーのテレポート先選定と移動処理 /
 * Teleport the player to a location up to "dis" grids away.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dis 基本移動距離
 * @param is_quantum_effect 量子的効果 (反テレポ無効)によるテレポートアウェイならばTRUE
 * @param mode オプション
 * @return 実際にテレポート処理が行われたらtrue
 * @details
 * <pre>
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 *
 * There was a nasty tendency for a long time; which was causing the
 * player to "bounce" between two or three different spots because
 * these are the only spots that are "far enough" way to satisfy the
 * algorithm.
 *
 * But this tendency is now removed; in the new algorithm, a list of
 * candidates is selected first, which includes at least 50% of all
 * floor grids within the distance, and any single grid in this list
 * of candidates has equal possibility to be choosen as a destination.
 * </pre>
 */
bool teleport_player_aux(player_type *player_ptr, POSITION dis, bool is_quantum_effect, teleport_flags mode)
{
    if (player_ptr->wild_mode)
        return false;
    if (!is_quantum_effect && player_ptr->anti_tele && !(mode & TELEPORT_NONMAGICAL)) {
        msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
        return false;
    }

    int candidates_at[MAX_TELEPORT_DISTANCE + 1];
    for (int i = 0; i <= MAX_TELEPORT_DISTANCE; i++)
        candidates_at[i] = 0;

    if (dis > MAX_TELEPORT_DISTANCE)
        dis = MAX_TELEPORT_DISTANCE;

    int left = MAX(1, player_ptr->x - dis);
    int right = MIN(player_ptr->current_floor_ptr->width - 2, player_ptr->x + dis);
    int top = MAX(1, player_ptr->y - dis);
    int bottom = MIN(player_ptr->current_floor_ptr->height - 2, player_ptr->y + dis);
    int total_candidates = 0;
    for (POSITION y = top; y <= bottom; y++) {
        for (POSITION x = left; x <= right; x++) {
            if (!cave_player_teleportable_bold(player_ptr, y, x, mode))
                continue;

            int d = distance(player_ptr->y, player_ptr->x, y, x);
            if (d > dis)
                continue;

            total_candidates++;
            candidates_at[d]++;
        }
    }

    if (0 == total_candidates)
        return false;

    int cur_candidates;
    int min = dis;
    for (cur_candidates = 0; min >= 0; min--) {
        cur_candidates += candidates_at[min];
        if (cur_candidates && (cur_candidates >= total_candidates / 2))
            break;
    }

    int pick = randint1(cur_candidates);

    /* Search again the choosen location */
    POSITION yy, xx = 0;
    for (yy = top; yy <= bottom; yy++) {
        for (xx = left; xx <= right; xx++) {
            if (!cave_player_teleportable_bold(player_ptr, yy, xx, mode))
                continue;

            int d = distance(player_ptr->y, player_ptr->x, yy, xx);
            if (d > dis)
                continue;
            if (d < min)
                continue;

            pick--;
            if (!pick)
                break;
        }

        if (!pick)
            break;
    }

    if (player_bold(player_ptr, yy, xx))
        return false;

    sound(SOUND_TELEPORT);
#ifdef JP
    if (is_echizen(player_ptr))
        msg_format("『こっちだぁ、%s』", player_ptr->name);
#endif
    (void)move_player_effect(player_ptr, yy, xx, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
    return true;
}

/*!
 * @brief プレイヤーのテレポート処理メインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dis 基本移動距離
 * @param mode オプション
 */
void teleport_player(player_type *player_ptr, POSITION dis, BIT_FLAGS mode)
{
    const POSITION oy = player_ptr->y;
    const POSITION ox = player_ptr->x;

    if (!teleport_player_aux(player_ptr, dis, false, i2enum<teleport_flags>(mode)))
        return;

    /* Monsters with teleport ability may follow the player */
    for (POSITION xx = -1; xx < 2; xx++) {
        for (POSITION yy = -1; yy < 2; yy++) {
            MONSTER_IDX tmp_m_idx = player_ptr->current_floor_ptr->grid_array[oy + yy][ox + xx].m_idx;
            if (tmp_m_idx && (player_ptr->riding != tmp_m_idx)) {
                monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[tmp_m_idx];
                monster_race *r_ptr = &r_info[m_ptr->r_idx];

                bool can_follow = r_ptr->ability_flags.has(RF_ABILITY::TPORT);
                can_follow &= none_bits(r_ptr->flagsr, RFR_RES_TELE);
                can_follow &= monster_csleep_remaining(m_ptr) == 0;
                if (can_follow) {
                    teleport_monster_to(player_ptr, tmp_m_idx, player_ptr->y, player_ptr->x, r_ptr->level, TELEPORT_SPONTANEOUS);
                }
            }
        }
    }
}

/*!
 * @brief プレイヤーのテレポートアウェイ処理 /
 * @param m_idx アウェイを試みたモンスターID
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dis テレポート距離
 * @param is_quantum_effect 量子的効果によるテレポートアウェイならばTRUE
 */
void teleport_player_away(MONSTER_IDX m_idx, player_type *player_ptr, POSITION dis, bool is_quantum_effect)
{
    if (player_ptr->phase_out)
        return;

    const POSITION oy = player_ptr->y;
    const POSITION ox = player_ptr->x;

    if (!teleport_player_aux(player_ptr, dis, is_quantum_effect, TELEPORT_PASSIVE))
        return;

    /* Monsters with teleport ability may follow the player */
    for (POSITION xx = -1; xx < 2; xx++) {
        for (POSITION yy = -1; yy < 2; yy++) {
            MONSTER_IDX tmp_m_idx = player_ptr->current_floor_ptr->grid_array[oy + yy][ox + xx].m_idx;
            bool is_teleportable = tmp_m_idx > 0;
            is_teleportable &= player_ptr->riding != tmp_m_idx;
            is_teleportable &= m_idx != tmp_m_idx;
            if (!is_teleportable) {
                continue;
            }

            monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[tmp_m_idx];
            monster_race *r_ptr = &r_info[m_ptr->r_idx];

            bool can_follow = r_ptr->ability_flags.has(RF_ABILITY::TPORT);
            can_follow &= none_bits(r_ptr->flagsr, RFR_RES_TELE);
            can_follow &= monster_csleep_remaining(m_ptr) == 0;
            if (can_follow) {
                teleport_monster_to(player_ptr, tmp_m_idx, player_ptr->y, player_ptr->x, r_ptr->level, TELEPORT_SPONTANEOUS);
            }
        }
    }
}

/*!
 * @brief プレイヤーを指定位置近辺にテレポートさせる
 * Teleport player to a grid near the given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ny 目標Y座標
 * @param nx 目標X座標
 * @param mode オプションフラグ
 * @details
 * <pre>
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 * </pre>
 */
void teleport_player_to(player_type *player_ptr, POSITION ny, POSITION nx, teleport_flags mode)
{
    if (player_ptr->anti_tele && !(mode & TELEPORT_NONMAGICAL)) {
        msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
        return;
    }

    /* Find a usable location */
    POSITION y, x;
    POSITION dis = 0, ctr = 0;
    while (true) {
        while (true) {
            y = (POSITION)rand_spread(ny, dis);
            x = (POSITION)rand_spread(nx, dis);
            if (in_bounds(player_ptr->current_floor_ptr, y, x))
                break;
        }

        bool is_anywhere = w_ptr->wizard;
        is_anywhere &= (mode & TELEPORT_PASSIVE) == 0;
        is_anywhere
            &= (player_ptr->current_floor_ptr->grid_array[y][x].m_idx > 0) || player_ptr->current_floor_ptr->grid_array[y][x].m_idx == player_ptr->riding;
        if (is_anywhere)
            break;

        if (cave_player_teleportable_bold(player_ptr, y, x, mode))
            break;

        if (++ctr > (4 * dis * dis + 4 * dis + 1)) {
            ctr = 0;
            dis++;
        }
    }

    sound(SOUND_TELEPORT);
    (void)move_player_effect(player_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
}

void teleport_away_followable(player_type *player_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    POSITION oldfy = m_ptr->fy;
    POSITION oldfx = m_ptr->fx;
    bool old_ml = m_ptr->ml;
    POSITION old_cdis = m_ptr->cdis;

    teleport_away(player_ptr, m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);

    bool is_followable = old_ml;
    is_followable &= old_cdis <= MAX_SIGHT;
    is_followable &= w_ptr->timewalk_m_idx == 0;
    is_followable &= !player_ptr->phase_out;
    is_followable &= los(player_ptr, player_ptr->y, player_ptr->x, oldfy, oldfx);
    if (!is_followable)
        return;

    bool follow = false;
    if (player_ptr->muta.has(MUTA::VTELEPORT) || (player_ptr->pclass == CLASS_IMITATOR))
        follow = true;
    else {
        object_type *o_ptr;
        INVENTORY_IDX i;

        for (i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
            o_ptr = &player_ptr->inventory_list[i];
            if (o_ptr->k_idx && !o_ptr->is_cursed()) {
                auto flgs = object_flags(o_ptr);
                if (flgs.has(TR_TELEPORT)) {
                    follow = true;
                    break;
                }
            }
        }
    }

    if (!follow)
        return;
    if (!get_check_strict(player_ptr, _("ついていきますか？", "Do you follow it? "), CHECK_OKAY_CANCEL))
        return;

    if (one_in_(3)) {
        teleport_player(player_ptr, 200, TELEPORT_PASSIVE);
        msg_print(_("失敗！", "Failed!"));
    } else {
        teleport_player_to(player_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_SPONTANEOUS);
    }

    player_ptr->energy_need += ENERGY_NEED();
}

/*!
 * @brief 次元の扉処理 /
 * Dimension Door
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param x テレポート先のX座標
 * @param y テレポート先のY座標
 * @return 目標に指定通りテレポートできたならばTRUEを返す
 */
bool exe_dimension_door(player_type *player_ptr, POSITION x, POSITION y)
{
    PLAYER_LEVEL plev = player_ptr->lev;

    player_ptr->energy_need += (int16_t)((int32_t)(60 - plev) * ENERGY_NEED() / 100L);

    if (!cave_player_teleportable_bold(player_ptr, y, x, TELEPORT_SPONTANEOUS) || (distance(y, x, player_ptr->y, player_ptr->x) > plev / 2 + 10)
        || (!randint0(plev / 10 + 10))) {
        player_ptr->energy_need += (int16_t)((int32_t)(60 - plev) * ENERGY_NEED() / 100L);
        teleport_player(player_ptr, (plev + 2) * 2, TELEPORT_PASSIVE);
        return false;
    }

    teleport_player_to(player_ptr, y, x, TELEPORT_SPONTANEOUS);
    return true;
}

/*!
 * @brief 次元の扉処理のメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 * Dimension Door
 * @return ターンを消費した場合TRUEを返す
 */
bool dimension_door(player_type *player_ptr)
{
    DEPTH x = 0, y = 0;
    if (!tgt_pt(player_ptr, &x, &y))
        return false;

    if (exe_dimension_door(player_ptr, x, y))
        return true;

    msg_print(_("精霊界から物質界に戻る時うまくいかなかった！", "You fail to exit the astral plane correctly!"));
    return true;
}
