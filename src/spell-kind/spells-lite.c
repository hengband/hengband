#include "spell-kind/spells-lite.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * todo この辺、xとyが引数になっているが、caster_ptr->xとcaster_ptr->yで全て置き換えが効くはず……
 * @brief 部屋全体を照らすサブルーチン
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This routine clears the entire "temp" set.
 * This routine will Perma-Lite all "temp" grids.
 * This routine is used (only) by "lite_room()"
 * Dark grids are illuminated.
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 1/4 the time when illuminated
 * STUPID monsters wake up 1/10 the time when illuminated
 * </pre>
 */
static void cave_temp_room_lite(player_type *caster_ptr)
{
    for (int i = 0; i < tmp_pos.n; i++) {
        POSITION y = tmp_pos.y[i];
        POSITION x = tmp_pos.x[i];
        grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_TEMP);
        g_ptr->info |= (CAVE_GLOW);
        if (g_ptr->m_idx) {
            PERCENTAGE chance = 25;
            monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
            monster_race *r_ptr = &r_info[m_ptr->r_idx];
            update_monster(caster_ptr, g_ptr->m_idx, FALSE);
            if (r_ptr->flags2 & (RF2_STUPID))
                chance = 10;
            if (r_ptr->flags2 & (RF2_SMART))
                chance = 100;

            if (monster_csleep_remaining(m_ptr) && (randint0(100) < chance)) {
                (void)set_monster_csleep(caster_ptr, g_ptr->m_idx, 0);
                if (m_ptr->ml) {
                    GAME_TEXT m_name[MAX_NLEN];
                    monster_desc(caster_ptr, m_name, m_ptr, 0);
                    msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
                }
            }
        }

        note_spot(caster_ptr, y, x);
        lite_spot(caster_ptr, y, x);
        update_local_illumination(caster_ptr, y, x);
    }

    tmp_pos.n = 0;
}

/*!
 * todo この辺、xとyが引数になっているが、caster_ptr->xとcaster_ptr->yで全て置き換えが効くはず……
 * @brief 部屋全体を暗くするサブルーチン
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This routine clears the entire "temp" set.
 * This routine will "darken" all "temp" grids.
 * In addition, some of these grids will be "unmarked".
 * This routine is used (only) by "unlite_room()"
 * Also, process all affected monsters
 * </pre>
 */
static void cave_temp_room_unlite(player_type *caster_ptr)
{
    for (int i = 0; i < tmp_pos.n; i++) {
        POSITION y = tmp_pos.y[i];
        POSITION x = tmp_pos.x[i];
        grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
        bool do_dark = !is_mirror_grid(g_ptr);
        g_ptr->info &= ~(CAVE_TEMP);
        if (!do_dark)
            continue;

        if (caster_ptr->current_floor_ptr->dun_level || !is_daytime()) {
            for (int j = 0; j < 9; j++) {
                POSITION by = y + ddy_ddd[j];
                POSITION bx = x + ddx_ddd[j];

                if (in_bounds2(caster_ptr->current_floor_ptr, by, bx)) {
                    grid_type *cc_ptr = &caster_ptr->current_floor_ptr->grid_array[by][bx];

                    if (has_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW)) {
                        do_dark = FALSE;
                        break;
                    }
                }
            }

            if (!do_dark)
                continue;
        }

        g_ptr->info &= ~(CAVE_GLOW);
        if (!has_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_REMEMBER)) {
            if (!view_torch_grids)
                g_ptr->info &= ~(CAVE_MARK);
            note_spot(caster_ptr, y, x);
        }

        if (g_ptr->m_idx) {
            update_monster(caster_ptr, g_ptr->m_idx, FALSE);
        }

        lite_spot(caster_ptr, y, x);
        update_local_illumination(caster_ptr, y, x);
    }

    tmp_pos.n = 0;
}

/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する / Determine how much contiguous open space this grid is next to
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param cy Y座標
 * @param cx X座標
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static int next_to_open(floor_type *floor_ptr, POSITION cy, POSITION cx, bool (*pass_bold)(floor_type *, POSITION, POSITION))
{
    int len = 0;
    int blen = 0;
    for (int i = 0; i < 16; i++) {
        POSITION y = cy + ddy_cdd[i % 8];
        POSITION x = cx + ddx_cdd[i % 8];
        if (!pass_bold(floor_ptr, y, x)) {
            if (len > blen) {
                blen = len;
            }

            len = 0;
        } else {
            len++;
        }
    }

    return MAX(len, blen);
}

/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する / Determine how much contiguous open space this grid is next to
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param cy Y座標
 * @param cx X座標
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static int next_to_walls_adj(floor_type *floor_ptr, POSITION cy, POSITION cx, bool (*pass_bold)(floor_type *, POSITION, POSITION))
{
    POSITION y, x;
    int c = 0;
    for (DIRECTION i = 0; i < 8; i++) {
        y = cy + ddy_ddd[i];
        x = cx + ddx_ddd[i];

        if (!pass_bold(floor_ptr, y, x))
            c++;
    }

    return c;
}

/*!
 * @brief 部屋内にある一点の周囲に該当する地形数かいくつあるかをグローバル変数tmp_pos.nに返す / Aux function -- see below
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 部屋内のy座標1点
 * @param x 部屋内のx座標1点
 * @param only_room 部屋内地形のみをチェック対象にするならば TRUE
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static void cave_temp_room_aux(player_type *caster_ptr, POSITION y, POSITION x, bool only_room, bool (*pass_bold)(floor_type *, POSITION, POSITION))
{
    grid_type *g_ptr;
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->info & (CAVE_TEMP))
        return;

    if (!(g_ptr->info & (CAVE_ROOM))) {
        if (only_room)
            return;
        if (!in_bounds2(floor_ptr, y, x))
            return;
        if (distance(caster_ptr->y, caster_ptr->x, y, x) > get_max_range(caster_ptr))
            return;

        /* Verify this grid */
        /*
         * The reason why it is ==6 instead of >5 is that 8 is impossible
         * due to the check for cave_bold above.
         * 7 lights dead-end corridors (you need to do this for the
         * checkboard interesting rooms, so that the boundary is lit
         * properly.
         * This leaves only a check for 6 bounding walls!
         */
        if (in_bounds(floor_ptr, y, x) && pass_bold(floor_ptr, y, x) && (next_to_walls_adj(floor_ptr, y, x, pass_bold) == 6)
            && (next_to_open(floor_ptr, y, x, pass_bold) <= 1))
            return;
    }

    if (tmp_pos.n == TEMP_MAX)
        return;

    g_ptr->info |= (CAVE_TEMP);
    tmp_pos.y[tmp_pos.n] = y;
    tmp_pos.x[tmp_pos.n] = x;
    tmp_pos.n++;
}

/*!
 * @brief 部屋内にある一点の周囲がいくつ光を通すかをグローバル変数tmp_pos.nに返す / Aux function -- see below
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return なし
 */
static void cave_temp_lite_room_aux(player_type *caster_ptr, POSITION y, POSITION x) { cave_temp_room_aux(caster_ptr, y, x, FALSE, cave_los_bold); }

/*!
 * @brief 指定のマスが光を通さず射線のみを通すかを返す。 / Aux function -- see below
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 射線を通すならばtrueを返す。
 */
static bool cave_pass_dark_bold(floor_type *floor_ptr, POSITION y, POSITION x) { return cave_has_flag_bold(floor_ptr, y, x, FF_PROJECT); }

/*!
 * @brief 部屋内にある一点の周囲がいくつ射線を通すかをグローバル変数tmp_pos.nに返す / Aux function -- see below
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return なし
 */
static void cave_temp_unlite_room_aux(player_type *caster_ptr, POSITION y, POSITION x) { cave_temp_room_aux(caster_ptr, y, x, TRUE, cave_pass_dark_bold); }

/*!
 * @brief 指定された部屋内を照らす / Illuminate any room containing the given location.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y1 指定Y座標
 * @param x1 指定X座標
 * @return なし
 */
void lite_room(player_type *caster_ptr, POSITION y1, POSITION x1)
{
    cave_temp_lite_room_aux(caster_ptr, y1, x1);
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    for (int i = 0; i < tmp_pos.n; i++) {
        POSITION x = tmp_pos.x[i];
        POSITION y = tmp_pos.y[i];

        if (!cave_los_bold(floor_ptr, y, x))
            continue;

        cave_temp_lite_room_aux(caster_ptr, y + 1, x);
        cave_temp_lite_room_aux(caster_ptr, y - 1, x);
        cave_temp_lite_room_aux(caster_ptr, y, x + 1);
        cave_temp_lite_room_aux(caster_ptr, y, x - 1);

        cave_temp_lite_room_aux(caster_ptr, y + 1, x + 1);
        cave_temp_lite_room_aux(caster_ptr, y - 1, x - 1);
        cave_temp_lite_room_aux(caster_ptr, y - 1, x + 1);
        cave_temp_lite_room_aux(caster_ptr, y + 1, x - 1);
    }

    cave_temp_room_lite(caster_ptr);
    if (caster_ptr->special_defense & NINJA_S_STEALTH) {
        if (floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info & CAVE_GLOW)
            set_superstealth(caster_ptr, FALSE);
    }
}

/*!
 * @brief 指定された部屋内を暗くする / Darken all rooms containing the given location
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y1 指定Y座標
 * @param x1 指定X座標
 * @return なし
 */
void unlite_room(player_type *caster_ptr, POSITION y1, POSITION x1)
{
    cave_temp_unlite_room_aux(caster_ptr, y1, x1);
    for (int i = 0; i < tmp_pos.n; i++) {
        POSITION x = tmp_pos.x[i];
        POSITION y = tmp_pos.y[i];
        if (!cave_pass_dark_bold(caster_ptr->current_floor_ptr, y, x))
            continue;

        cave_temp_unlite_room_aux(caster_ptr, y + 1, x);
        cave_temp_unlite_room_aux(caster_ptr, y - 1, x);
        cave_temp_unlite_room_aux(caster_ptr, y, x + 1);
        cave_temp_unlite_room_aux(caster_ptr, y, x - 1);

        cave_temp_unlite_room_aux(caster_ptr, y + 1, x + 1);
        cave_temp_unlite_room_aux(caster_ptr, y - 1, x - 1);
        cave_temp_unlite_room_aux(caster_ptr, y - 1, x + 1);
        cave_temp_unlite_room_aux(caster_ptr, y + 1, x - 1);
    }

    cave_temp_room_unlite(caster_ptr);
}

/*!
 * @brief スターライトの効果を発生させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param magic 魔法による効果であればTRUE、スターライトの杖による効果であればFALSE
 * @return 常にTRUE
 */
bool starlight(player_type *caster_ptr, bool magic)
{
    if (!caster_ptr->blind && !magic) {
        msg_print(_("杖の先が明るく輝いた...", "The end of the staff glows brightly..."));
    }

    HIT_POINT num = damroll(5, 3);
    int attempts;
    POSITION y = 0, x = 0;
    for (int k = 0; k < num; k++) {
        attempts = 1000;

        while (attempts--) {
            scatter(caster_ptr, &y, &x, caster_ptr->y, caster_ptr->x, 4, PROJECT_LOS);
            if (!cave_has_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT))
                continue;
            if (!player_bold(caster_ptr, y, x))
                break;
        }

        project(caster_ptr, 0, 0, y, x, damroll(6 + caster_ptr->lev / 8, 10), GF_LITE_WEAK,
            (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL | PROJECT_LOS), -1);
    }

    return TRUE;
}

/*!
 * @brief プレイヤー位置を中心にLITE_WEAK属性を通じた照明処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_area(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
    if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) {
        msg_print(_("ダンジョンが光を吸収した。", "The darkness of this dungeon absorbs your light."));
        return FALSE;
    }

    if (!caster_ptr->blind) {
        msg_print(_("白い光が辺りを覆った。", "You are surrounded by a white light."));
    }

    BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;
    (void)project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, dam, GF_LITE_WEAK, flg, -1);

    lite_room(caster_ptr, caster_ptr->y, caster_ptr->x);

    return TRUE;
}

/*!
 * @brief プレイヤー位置を中心にLITE_DARK属性を通じた消灯処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool unlite_area(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
    if (!caster_ptr->blind) {
        msg_print(_("暗闇が辺りを覆った。", "Darkness surrounds you."));
    }

    BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;
    (void)project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, dam, GF_DARK_WEAK, flg, -1);

    unlite_room(caster_ptr, caster_ptr->y, caster_ptr->x);

    return TRUE;
}

/*!
 * @brief LITE_WEAK属性による光源ビーム処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_line(player_type *caster_ptr, DIRECTION dir, HIT_POINT dam)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
    return (project_hook(caster_ptr, GF_LITE_WEAK, dir, dam, flg));
}
