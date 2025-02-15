#include "floor/tunnel-generator.h"
#include "floor/dungeon-tunnel-util.h"
#include "grid/grid.h"
#include "system/dungeon/dungeon-data-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief build_tunnel用に通路を掘るための方向をランダムに決める
 * @return トンネルの方向
 */
static Pos2DVec rand_dir()
{
    const auto d = rand_choice(Direction::directions_4());
    return d.vec();
}

/*!
 * @brief build_tunnel用に通路を掘るための方向を位置関係通りに決める
 * @param pos_start 始点座標
 * @param pos_end 終点座標
 * @return トンネルの方向
 */
static Pos2DVec correct_dir(const Pos2D &pos_start, const Pos2D &pos_end)
{
    auto y = (pos_start.y == pos_end.y) ? 0 : (pos_start.y < pos_end.y) ? 1
                                                                        : -1;
    auto x = (pos_start.x == pos_end.x) ? 0 : (pos_start.x < pos_end.x) ? 1
                                                                        : -1;
    if ((y == 0) || (x == 0)) {
        return { y, x };
    }

    if (one_in_(2)) {
        y = 0;
    } else {
        x = 0;
    }

    return { y, x };
}

/*!
 * @brief 部屋間のトンネルを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos_start 始点
 * @param pos_end 終点
 * @return 生成に成功したらTRUEを返す
 */
bool build_tunnel(PlayerType *player_ptr, DungeonData *dd_ptr, dt_type *dt_ptr, const Pos2D &pos_start, const Pos2D &pos_end)
{
    Pos2D pos_current = pos_start;
    auto main_loop_count = 0;
    auto door_flag = false;
    auto vec = correct_dir(pos_start, pos_end);
    auto &floor = *player_ptr->current_floor_ptr;
    while (pos_current != pos_end) {
        if (main_loop_count++ > 2000) {
            return false;
        }

        if (evaluate_percent(dt_ptr->dun_tun_chg)) {
            vec = correct_dir(pos_current, pos_end);
            if (evaluate_percent(dt_ptr->dun_tun_rnd)) {
                vec = rand_dir();
            }
        }

        auto pos_tmp = pos_current + vec;
        while (!floor.contains(pos_tmp)) {
            vec = correct_dir(pos_current, pos_end);
            if (evaluate_percent(dt_ptr->dun_tun_rnd)) {
                vec = rand_dir();
            }

            pos_tmp = pos_current + vec;
        }

        const auto &grid_tmp = floor.get_grid(pos_tmp);
        if (grid_tmp.is_solid()) {
            continue;
        }

        if (grid_tmp.is_outer()) {
            const auto pos = pos_tmp + vec;
            auto &grid = floor.get_grid(pos);
            if (grid.is_outer() || grid.is_solid()) {
                continue;
            }

            pos_current = pos_tmp;
            if (dd_ptr->wall_n >= dd_ptr->walls.size()) {
                return false;
            }

            dd_ptr->walls[dd_ptr->wall_n] = pos_current;
            dd_ptr->wall_n++;
            for (auto y = pos_current.y - 1; y <= pos_current.y + 1; y++) {
                for (auto x = pos_current.x - 1; x <= pos_current.x + 1; x++) {
                    const Pos2D pos_wall(y, x);
                    if (floor.get_grid(pos_wall).is_outer()) {
                        place_bold(player_ptr, pos_wall.y, pos_wall.x, GB_SOLID_NOPERM);
                    }
                }
            }

            continue;
        }

        if (grid_tmp.info & (CAVE_ROOM)) {
            pos_current = pos_tmp;
            continue;
        }

        if (grid_tmp.is_extra() || grid_tmp.is_inner() || grid_tmp.is_solid()) {
            pos_current = pos_tmp;
            if (dd_ptr->tunn_n >= dd_ptr->tunnels.size()) {
                return false;
            }

            dd_ptr->tunnels[dd_ptr->tunn_n] = pos_current;
            dd_ptr->tunn_n++;
            door_flag = false;
            continue;
        }

        pos_current = pos_tmp;
        if (!door_flag) {
            if (dd_ptr->door_n >= dd_ptr->doors.size()) {
                return false;
            }

            dd_ptr->doors[dd_ptr->door_n] = pos_current;
            dd_ptr->door_n++;
            door_flag = true;
        }

        if (evaluate_percent(dt_ptr->dun_tun_con)) {
            continue;
        }

        const auto vec_current = pos_current - pos_start;
        if ((std::abs(vec_current.y) > 10) || (std::abs(vec_current.x) > 10)) {
            break;
        }
    }

    return true;
}

/*!
 * @brief トンネル生成のための基準点を指定する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param x 基準点を指定するX座標の参照ポインタ、適時値が修正される。
 * @param y 基準点を指定するY座標の参照ポインタ、適時値が修正される。
 * @param affectwall (調査中)
 * @todo 特に詳細な処理の意味を調査すべし
 */
static bool set_tunnel(PlayerType *player_ptr, DungeonData *dd_ptr, POSITION *y, POSITION *x, bool affectwall)
{
    const Pos2D pos(*y, *x);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    if (!floor.contains(pos) || grid.is_inner()) {
        return true;
    }

    if (grid.is_extra()) {
        if (dd_ptr->tunn_n >= dd_ptr->tunnels.size()) {
            return false;
        }

        dd_ptr->tunnels[dd_ptr->tunn_n] = { *y, *x };
        dd_ptr->tunn_n++;
        return true;
    }

    if (grid.is_floor()) {
        return true;
    }

    if (grid.is_outer() && affectwall) {
        if (dd_ptr->wall_n >= dd_ptr->walls.size()) {
            return false;
        }

        dd_ptr->walls[dd_ptr->wall_n] = pos;
        dd_ptr->wall_n++;
        for (auto j = *y - 1; j <= *y + 1; j++) {
            for (auto i = *x - 1; i <= *x + 1; i++) {
                if (floor.get_grid({ j, i }).is_outer()) {
                    place_bold(player_ptr, j, i, GB_SOLID_NOPERM);
                }
            }
        }

        grid.mimic = 0;
        place_bold(player_ptr, *y, *x, GB_FLOOR);
        return true;
    }

    if (grid.is_solid() && affectwall) {
        auto i = 50;
        Pos2DVec vec(0, 0);
        while ((i > 0) && floor.get_grid(pos + vec).is_solid()) {
            vec.y = randint0(3) - 1;
            vec.x = randint0(3) - 1;
            if (!floor.contains(pos + vec)) {
                vec = { 0, 0 };
            }

            i--;
        }

        if (i == 0) {
            place_grid(player_ptr, grid, GB_OUTER);
            vec = { 0, 0 };
        }

        *x += vec.x;
        *y += vec.y;
        return false;
    }

    return true;
}

/*!
 * @brief 外壁を削って「カタコンベ状」の通路を作成する / This routine creates the catacomb-like tunnels by removing extra rock.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param x 基準点のX座標
 * @param y 基準点のY座標
 */
static void create_cata_tunnel(PlayerType *player_ptr, DungeonData *dd_ptr, POSITION x, POSITION y)
{
    auto x1 = x - 1;
    auto y1 = y;
    set_tunnel(player_ptr, dd_ptr, &y1, &x1, false);

    x1 = x + 1;
    y1 = y;
    set_tunnel(player_ptr, dd_ptr, &y1, &x1, false);

    x1 = x;
    y1 = y - 1;
    set_tunnel(player_ptr, dd_ptr, &y1, &x1, false);

    x1 = x;
    y1 = y + 1;
    set_tunnel(player_ptr, dd_ptr, &y1, &x1, false);
}

/*!
 * @brief トンネル生成処理（詳細調査中）/ This routine does the bulk of the work in creating the new types of tunnels.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @todo 詳細用調査
 */
static void short_seg_hack(
    PlayerType *player_ptr, DungeonData *dd_ptr, const POSITION x1, const POSITION y1, const POSITION x2, const POSITION y2, int type, int count, bool *fail)
{
    if (!(*fail)) {
        return;
    }

    const auto length = Grid::calc_distance({ x1, y1 }, { x2, y2 });
    count++;
    POSITION x, y;
    if ((type == 1) && (length != 0)) {
        for (int i = 0; i <= length; i++) {
            x = x1 + i * (x2 - x1) / length;
            y = y1 + i * (y2 - y1) / length;
            if (!set_tunnel(player_ptr, dd_ptr, &y, &x, true)) {
                if (count > 50) {
                    *fail = false;
                    return;
                }

                short_seg_hack(player_ptr, dd_ptr, x, y, x1 + (i - 1) * (x2 - x1) / length, y1 + (i - 1) * (y2 - y1) / length, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, x1 + (i + 1) * (x2 - x1) / length, y1 + (i + 1) * (y2 - y1) / length, 1, count, fail);
            }
        }

        return;
    }

    if ((type != 2) && (type != 3)) {
        return;
    }

    if (x1 < x2) {
        for (int i = x1; i <= x2; i++) {
            x = i;
            y = y1;
            if (!set_tunnel(player_ptr, dd_ptr, &y, &x, true)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, i - 1, y1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, i + 1, y1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2)) {
                create_cata_tunnel(player_ptr, dd_ptr, i, y1);
            }
        }
    } else {
        for (int i = x2; i <= x1; i++) {
            x = i;
            y = y1;
            if (!set_tunnel(player_ptr, dd_ptr, &y, &x, true)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, i - 1, y1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, i + 1, y1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2)) {
                create_cata_tunnel(player_ptr, dd_ptr, i, y1);
            }
        }
    }

    if (y1 < y2) {
        for (int i = y1; i <= y2; i++) {
            x = x2;
            y = i;
            if (!set_tunnel(player_ptr, dd_ptr, &y, &x, true)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i - 1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i + 1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2)) {
                create_cata_tunnel(player_ptr, dd_ptr, x2, i);
            }
        }
    } else {
        for (int i = y2; i <= y1; i++) {
            x = x2;
            y = i;
            if (!set_tunnel(player_ptr, dd_ptr, &y, &x, true)) {
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i - 1, 1, count, fail);
                short_seg_hack(player_ptr, dd_ptr, x, y, x2, i + 1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2)) {
                create_cata_tunnel(player_ptr, dd_ptr, x2, i);
            }
        }
    }
}

/*!
 * @brief 特定の壁(永久壁など)を避けながら部屋間の通路を作成する / This routine maps a path from (x1, y1) to (x2, y2) avoiding SOLID walls.
 * @todo 詳細要調査
 */
bool build_tunnel2(PlayerType *player_ptr, DungeonData *dd_ptr, const Pos2D &pos_start, const Pos2D &pos_end, int type, int cutoff)
{
    const auto length = Grid::calc_distance(pos_start, pos_end);
    auto &floor = *player_ptr->current_floor_ptr;
    if (length <= cutoff) {
        auto initial_failure = true;
        short_seg_hack(player_ptr, dd_ptr, pos_start.x, pos_start.y, pos_end.x, pos_end.y, type, 0, &initial_failure);
        return true;
    }

    auto vec = pos_end - pos_start;
    vec = Pos2DVec(vec.y / 2, vec.x / 2);
    const auto changex = (randint0(std::abs(vec.y) + 2) * 2 - std::abs(vec.y) - 1) / 2;
    const auto changey = (randint0(std::abs(vec.x) + 2) * 2 - std::abs(vec.x) - 1) / 2;
    auto pos = pos_start + vec;
    pos += Pos2DVec(changey, changex);
    if (!floor.contains(pos)) {
        pos.x = (pos_start.x + pos_end.x) / 2;
        pos.y = (pos_start.y + pos_end.y) / 2;
    }

    const auto *grid_ptr = &floor.get_grid(pos);
    if (grid_ptr->is_solid()) {
        auto i = 50;
        vec = { 0, 0 };
        while ((i > 0) && floor.get_grid(pos + vec).is_solid()) {
            const auto tmp_y = randint0(3) - 1;
            const auto tmp_x = randint0(3) - 1;
            vec = { tmp_y, tmp_x }; //!< @details 乱数引数の評価順を固定する.
            const auto pos_tmp = pos + vec;
            if (!floor.contains(pos_tmp)) {
                vec = { 0, 0 };
            }

            i--;
        }

        if (i == 0) {
            place_bold(player_ptr, pos.y, pos.x, GB_OUTER);
            vec = { 0, 0 };
        }

        pos += vec;
        grid_ptr = &floor.get_grid(pos);
    }

    bool is_tunnel_built;
    bool is_successful;
    if (grid_ptr->is_floor()) {
        if (build_tunnel2(player_ptr, dd_ptr, pos_start, pos, type, cutoff)) {
            if (floor.get_grid(pos).is_room() || (randint1(100) > 95)) {
                is_tunnel_built = build_tunnel2(player_ptr, dd_ptr, pos, pos_end, type, cutoff);
            } else {
                is_tunnel_built = false;
                if (dd_ptr->door_n >= dd_ptr->doors.size()) {
                    return false;
                }

                dd_ptr->doors[dd_ptr->door_n] = pos;
                dd_ptr->door_n++;
            }

            is_successful = true;
        } else {
            is_tunnel_built = false;
            is_successful = false;
        }
    } else {
        if (build_tunnel2(player_ptr, dd_ptr, pos_start, pos, type, cutoff)) {
            is_tunnel_built = build_tunnel2(player_ptr, dd_ptr, pos, pos_end, type, cutoff);
            is_successful = true;
        } else {
            is_tunnel_built = false;
            is_successful = false;
        }
    }

    if (is_successful) {
        (void)set_tunnel(player_ptr, dd_ptr, &pos.y, &pos.x, true);
    }

    return is_tunnel_built;
}
