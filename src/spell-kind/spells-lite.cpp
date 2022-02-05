#include "spell-kind/spells-lite.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "grid/feature.h"
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
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "util/point-2d.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <vector>

using PassBoldFunc = bool (*)(floor_type *, POSITION, POSITION);

/*!
 * @brief 指定した座標全てを照らす。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param points 明るくすべき座標たち
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
 * @todo この辺、xとyが引数になっているが、player_ptr->xとplayer_ptr->yで全て置き換えが効くはず……
 */
static void cave_temp_room_lite(PlayerType *player_ptr, const std::vector<Pos2D> &points)
{
    for (const auto &point : points) {
        const POSITION y = point.y;
        const POSITION x = point.x;

        grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
        g_ptr->info &= ~(CAVE_TEMP);
        g_ptr->info |= (CAVE_GLOW);
        if (g_ptr->m_idx) {
            PERCENTAGE chance = 25;
            monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
            monster_race *r_ptr = &r_info[m_ptr->r_idx];
            update_monster(player_ptr, g_ptr->m_idx, false);
            if (r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID))
                chance = 10;
            if (r_ptr->behavior_flags.has(MonsterBehaviorType::SMART))
                chance = 100;

            if (monster_csleep_remaining(m_ptr) && (randint0(100) < chance)) {
                (void)set_monster_csleep(player_ptr, g_ptr->m_idx, 0);
                if (m_ptr->ml) {
                    GAME_TEXT m_name[MAX_NLEN];
                    monster_desc(player_ptr, m_name, m_ptr, 0);
                    msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
                }
            }
        }

        note_spot(player_ptr, y, x);
        lite_spot(player_ptr, y, x);
        update_local_illumination(player_ptr, y, x);
    }
}

/*!
 * @brief 指定した座標全てを暗くする。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param points 暗くすべき座標たち
 * @details
 * <pre>
 * This routine clears the entire "temp" set.
 * This routine will "darken" all "temp" grids.
 * In addition, some of these grids will be "unmarked".
 * This routine is used (only) by "unlite_room()"
 * Also, process all affected monsters
 * </pre>
 * @todo この辺、xとyが引数になっているが、player_ptr->xとplayer_ptr->yで全て置き換えが効くはず……
 */
static void cave_temp_room_unlite(PlayerType *player_ptr, const std::vector<Pos2D> &points)
{
    for (const auto &point : points) {
        const POSITION y = point.y;
        const POSITION x = point.x;

        grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
        bool do_dark = !g_ptr->is_mirror();
        g_ptr->info &= ~(CAVE_TEMP);
        if (!do_dark)
            continue;

        if (player_ptr->current_floor_ptr->dun_level || !is_daytime()) {
            for (int j = 0; j < 9; j++) {
                POSITION by = y + ddy_ddd[j];
                POSITION bx = x + ddx_ddd[j];

                if (in_bounds2(player_ptr->current_floor_ptr, by, bx)) {
                    grid_type *cc_ptr = &player_ptr->current_floor_ptr->grid_array[by][bx];

                    if (f_info[cc_ptr->get_feat_mimic()].flags.has(FloorFeatureType::GLOW)) {
                        do_dark = false;
                        break;
                    }
                }
            }

            if (!do_dark)
                continue;
        }

        g_ptr->info &= ~(CAVE_GLOW);
        if (f_info[g_ptr->get_feat_mimic()].flags.has_not(FloorFeatureType::REMEMBER)) {
            if (!view_torch_grids)
                g_ptr->info &= ~(CAVE_MARK);
            note_spot(player_ptr, y, x);
        }

        if (g_ptr->m_idx) {
            update_monster(player_ptr, g_ptr->m_idx, false);
        }

        lite_spot(player_ptr, y, x);
        update_local_illumination(player_ptr, y, x);
    }
}

/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する / Determine how much contiguous open space this grid is next to
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param cy Y座標
 * @param cx X座標
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static int next_to_open(floor_type *floor_ptr, const POSITION cy, const POSITION cx, const PassBoldFunc pass_bold)
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

    return std::max(len, blen);
}

/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する / Determine how much contiguous open space this grid is next to
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param cy Y座標
 * @param cx X座標
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static int next_to_walls_adj(floor_type *floor_ptr, const POSITION cy, const POSITION cx, const PassBoldFunc pass_bold)
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
 * @brief (y,x) が指定条件を満たすなら points に加える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param points 座標記録用配列
 * @param y 部屋内のy座標1点
 * @param x 部屋内のx座標1点
 * @param only_room 部屋内地形のみをチェック対象にするならば TRUE
 * @param pass_bold 地形条件を返す関数ポインタ
 */
static void cave_temp_room_aux(
    PlayerType *player_ptr, std::vector<Pos2D> &points, const POSITION y, const POSITION x, const bool only_room, const PassBoldFunc pass_bold)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];

    // 既に points に追加済みなら何もしない。
    if (g_ptr->info & (CAVE_TEMP))
        return;

    if (!(g_ptr->info & (CAVE_ROOM))) {
        if (only_room)
            return;
        if (!in_bounds2(floor_ptr, y, x))
            return;
        if (distance(player_ptr->y, player_ptr->x, y, x) > get_max_range(player_ptr))
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
        if (in_bounds(floor_ptr, y, x) && pass_bold(floor_ptr, y, x) && (next_to_walls_adj(floor_ptr, y, x, pass_bold) == 6) && (next_to_open(floor_ptr, y, x, pass_bold) <= 1))
            return;
    }

    // (y,x) を points に追加し、追加済みフラグを立てる。
    points.emplace_back(y, x);
    g_ptr->info |= (CAVE_TEMP);
}

/*!
 * @brief (y,x) が明るくすべきマスなら points に加える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param points 座標記録用配列
 * @param y 指定Y座標
 * @param x 指定X座標
 */
static void cave_temp_lite_room_aux(PlayerType *player_ptr, std::vector<Pos2D> &points, const POSITION y, const POSITION x)
{
    cave_temp_room_aux(player_ptr, points, y, x, false, cave_los_bold);
}

/*!
 * @brief 指定のマスが光を通さず射線のみを通すかを返す。 / Aux function -- see below
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 射線を通すならばtrueを返す。
 */
static bool cave_pass_dark_bold(floor_type *floor_ptr, POSITION y, POSITION x)
{
    return cave_has_flag_bold(floor_ptr, y, x, FloorFeatureType::PROJECT);
}

/*!
 * @brief (y,x) が暗くすべきマスなら points に加える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 */
static void cave_temp_unlite_room_aux(PlayerType *player_ptr, std::vector<Pos2D> &points, const POSITION y, const POSITION x)
{
    cave_temp_room_aux(player_ptr, points, y, x, true, cave_pass_dark_bold);
}

/*!
 * @brief (y1,x1) を含む全ての部屋を照らす。 / Illuminate any room containing the given location.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 指定Y座標
 * @param x1 指定X座標
 *
 * NOTE: 部屋に限らないかも?
 */
void lite_room(PlayerType *player_ptr, const POSITION y1, const POSITION x1)
{
    // 明るくするマスを記録する配列。
    std::vector<Pos2D> points;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;

    // (y1,x1) を起点として明るくするマスを記録していく。
    // 実質幅優先探索。
    cave_temp_lite_room_aux(player_ptr, points, y1, x1);
    for (size_t i = 0; i < size(points); i++) {
        const auto &point = points[i];
        const POSITION y = point.y;
        const POSITION x = point.x;

        if (!cave_los_bold(floor_ptr, y, x))
            continue;

        cave_temp_lite_room_aux(player_ptr, points, y + 1, x);
        cave_temp_lite_room_aux(player_ptr, points, y - 1, x);
        cave_temp_lite_room_aux(player_ptr, points, y, x + 1);
        cave_temp_lite_room_aux(player_ptr, points, y, x - 1);

        cave_temp_lite_room_aux(player_ptr, points, y + 1, x + 1);
        cave_temp_lite_room_aux(player_ptr, points, y - 1, x - 1);
        cave_temp_lite_room_aux(player_ptr, points, y - 1, x + 1);
        cave_temp_lite_room_aux(player_ptr, points, y + 1, x - 1);
    }

    // 記録したマスを実際に明るくする。
    cave_temp_room_lite(player_ptr, points);

    // 超隠密状態の更新。
    if (floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }
}

/*!
 * @brief (y1,x1) を含む全ての部屋を暗くする。 / Darken all rooms containing the given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 指定Y座標
 * @param x1 指定X座標
 */
void unlite_room(PlayerType *player_ptr, const POSITION y1, const POSITION x1)
{
    // 暗くするマスを記録する配列。
    std::vector<Pos2D> points;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;

    // (y1,x1) を起点として暗くするマスを記録していく。
    // 実質幅優先探索。
    cave_temp_unlite_room_aux(player_ptr, points, y1, x1);
    for (size_t i = 0; i < size(points); i++) {
        const auto &point = points[i];
        const POSITION y = point.y;
        const POSITION x = point.x;

        if (!cave_pass_dark_bold(floor_ptr, y, x))
            continue;

        cave_temp_unlite_room_aux(player_ptr, points, y + 1, x);
        cave_temp_unlite_room_aux(player_ptr, points, y - 1, x);
        cave_temp_unlite_room_aux(player_ptr, points, y, x + 1);
        cave_temp_unlite_room_aux(player_ptr, points, y, x - 1);

        cave_temp_unlite_room_aux(player_ptr, points, y + 1, x + 1);
        cave_temp_unlite_room_aux(player_ptr, points, y - 1, x - 1);
        cave_temp_unlite_room_aux(player_ptr, points, y - 1, x + 1);
        cave_temp_unlite_room_aux(player_ptr, points, y + 1, x - 1);
    }

    // 記録したマスを実際に暗くする。
    cave_temp_room_unlite(player_ptr, points);
}

/*!
 * @brief スターライトの効果を発生させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param magic 魔法による効果であればTRUE、スターライトの杖による効果であればFALSE
 * @return 常にTRUE
 */
bool starlight(PlayerType *player_ptr, bool magic)
{
    if (!player_ptr->blind && !magic) {
        msg_print(_("杖の先が明るく輝いた...", "The end of the staff glows brightly..."));
    }

    HIT_POINT num = damroll(5, 3);
    int attempts;
    POSITION y = 0, x = 0;
    for (int k = 0; k < num; k++) {
        attempts = 1000;

        while (attempts--) {
            scatter(player_ptr, &y, &x, player_ptr->y, player_ptr->x, 4, PROJECT_LOS);
            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, FloorFeatureType::PROJECT))
                continue;
            if (!player_bold(player_ptr, y, x))
                break;
        }

        project(player_ptr, 0, 0, y, x, damroll(6 + player_ptr->lev / 8, 10), AttributeType::LITE_WEAK,
            (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL | PROJECT_LOS));
    }

    return true;
}

/*!
 * @brief プレイヤー位置を中心にLITE_WEAK属性を通じた照明処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_area(PlayerType *player_ptr, HIT_POINT dam, POSITION rad)
{
    if (d_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        msg_print(_("ダンジョンが光を吸収した。", "The darkness of this dungeon absorbs your light."));
        return false;
    }

    if (!player_ptr->blind) {
        msg_print(_("白い光が辺りを覆った。", "You are surrounded by a white light."));
    }

    BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;
    (void)project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, dam, AttributeType::LITE_WEAK, flg);

    lite_room(player_ptr, player_ptr->y, player_ptr->x);

    return true;
}

/*!
 * @brief プレイヤー位置を中心にLITE_DARK属性を通じた消灯処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool unlite_area(PlayerType *player_ptr, HIT_POINT dam, POSITION rad)
{
    if (!player_ptr->blind) {
        msg_print(_("暗闇が辺りを覆った。", "Darkness surrounds you."));
    }

    BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;
    (void)project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, dam, AttributeType::DARK_WEAK, flg);

    unlite_room(player_ptr, player_ptr->y, player_ptr->x);

    return true;
}

/*!
 * @brief LITE_WEAK属性による光源ビーム処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_line(PlayerType *player_ptr, DIRECTION dir, HIT_POINT dam)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
    return (project_hook(player_ptr, AttributeType::LITE_WEAK, dir, dam, flg));
}
