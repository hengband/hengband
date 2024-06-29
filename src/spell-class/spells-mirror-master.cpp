/*!
 * @brief 鏡使いの鏡魔法効果処理
 * @date 2022/03/07
 * @author Hourier
 * @todo 作りかけの部分複数あり
 */

#include "spell-class/spells-mirror-master.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-monster.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "monster/monster-update.h"
#include "pet/pet-util.h"
#include "spell-kind/spells-teleport.h"
#include "system/angband-system.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "target/grid-selector.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <algorithm>
#include <map>

SpellsMirrorMaster::SpellsMirrorMaster(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

void SpellsMirrorMaster::remove_mirror(int y, int x)
{
    auto &floor = *this->player_ptr->current_floor_ptr;
    auto *g_ptr = &floor.grid_array[y][x];
    reset_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = 0;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        reset_bits(g_ptr->info, CAVE_GLOW);
        if (!view_torch_grids) {
            reset_bits(g_ptr->info, CAVE_MARK);
        }

        if (g_ptr->has_monster()) {
            update_monster(this->player_ptr, g_ptr->m_idx, false);
        }

        update_local_illumination(this->player_ptr, y, x);
    }

    note_spot(this->player_ptr, y, x);
    lite_spot(this->player_ptr, y, x);
}

/*!
 * @brief 全鏡の消去 / Remove all mirrors in this floor
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param explode 爆発処理を伴うならばTRUE
 */
void SpellsMirrorMaster::remove_all_mirrors(bool explode)
{
    const auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto x = 0; x < floor_ptr->width; x++) {
        for (auto y = 0; y < floor_ptr->height; y++) {
            if (!floor_ptr->grid_array[y][x].is_mirror()) {
                continue;
            }

            this->remove_mirror(y, x);
            if (!explode) {
                continue;
            }

            constexpr BIT_FLAGS projection = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI;
            project(this->player_ptr, 0, 2, y, x, this->player_ptr->lev / 2 + 5, AttributeType::SHARDS, projection);
        }
    }
}

/*!
 * @brief 鏡抜け処理のメインルーチン /
 * Mirror Master's Dimension Door
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return ターンを消費した場合TRUEを返す
 */
bool SpellsMirrorMaster::mirror_tunnel()
{
    int x;
    int y;
    if (!tgt_pt(this->player_ptr, &x, &y)) {
        return false;
    }

    if (exe_dimension_door(this->player_ptr, x, y)) {
        return true;
    }

    msg_print(_("鏡の世界をうまく通れなかった！", "You could not enter the mirror!"));
    return true;
}

/*!
 * @brief 鏡設置処理
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool SpellsMirrorMaster::place_mirror()
{
    auto y = this->player_ptr->y;
    auto x = this->player_ptr->x;
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    if (!cave_clean_bold(floor_ptr, y, x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return false;
    }

    /* Create a mirror */
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    set_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = feat_mirror;

    /* Turn on the light */
    set_bits(g_ptr->info, CAVE_GLOW);

    note_spot(this->player_ptr, y, x);
    lite_spot(this->player_ptr, y, x);
    update_local_illumination(this->player_ptr, y, x);
    return true;
}

/*!
 * @brief 静水
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return ペットを操っている場合を除きTRUE
 */
bool SpellsMirrorMaster::mirror_concentration()
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return false;
    }

    if (!this->player_ptr->current_floor_ptr->grid_array[this->player_ptr->y][this->player_ptr->x].is_mirror()) {
        msg_print(_("鏡の上でないと集中できない！", "There's no mirror here!"));
        return true;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));
    this->player_ptr->csp += (5 + this->player_ptr->lev * this->player_ptr->lev / 100);
    if (this->player_ptr->csp >= this->player_ptr->msp) {
        this->player_ptr->csp = this->player_ptr->msp;
        this->player_ptr->csp_frac = 0;
    }

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
    return true;
}

/*!
 * @brief 鏡魔法「鏡の封印」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
void SpellsMirrorMaster::seal_of_mirror(const int dam)
{
    const auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto x = 0; x < floor_ptr->width; x++) {
        for (auto y = 0; y < floor_ptr->height; y++) {
            const auto &g_ref = floor_ptr->grid_array[y][x];
            if (!g_ref.is_mirror()) {
                continue;
            }

            constexpr BIT_FLAGS flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP;
            if (!affect_monster(this->player_ptr, 0, 0, y, x, dam, AttributeType::GENOCIDE, flags, true)) {
                continue;
            }

            if (!g_ref.has_monster()) {
                this->remove_mirror(y, x);
            }
        }
    }
}

void SpellsMirrorMaster::seeker_ray(int dir, int dam)
{
    const auto pos = ((dir == 5) && target_okay(this->player_ptr)) ? Pos2D(target_row, target_col) : this->player_ptr->get_neighbor(dir);
    project_seeker_ray(pos.x, pos.y, dam);
}

void SpellsMirrorMaster::super_ray(int dir, int dam)
{
    const auto pos = ((dir == 5) && target_okay(this->player_ptr)) ? Pos2D(target_row, target_col) : this->player_ptr->get_neighbor(dir);
    project_super_ray(pos.x, pos.y, dam);
}

/*!
 * @brief 配置した鏡リストの次を取得する
 * @param pos_current 現在の鏡位置
 * @return 次の鏡位置
 */
Pos2D SpellsMirrorMaster::get_next_mirror_position(const Pos2D &pos_current) const
{
    std::vector<Pos2D> mirror_positions;
    const auto &floor = *this->player_ptr->current_floor_ptr;
    for (auto x = 0; x < floor.width; x++) {
        for (auto y = 0; y < floor.height; y++) {
            const Pos2D pos(y, x);
            if (floor.get_grid(pos).is_mirror()) {
                mirror_positions.push_back(pos);
            }
        }
    }

    if (!mirror_positions.empty()) {
        return rand_choice(mirror_positions);
    }

    while (true) {
        const Pos2D next_mirror(pos_current.y + randint0(5) - 2, pos_current.x + randint0(5) - 2);
        if (next_mirror != pos_current) {
            return next_mirror;
        }
    }
}

void SpellsMirrorMaster::project_seeker_ray(int target_x, int target_y, int dam)
{
    constexpr auto typ = AttributeType::SEEKER;
    BIT_FLAGS flag = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_THRU | PROJECT_MIRROR;
    rakubadam_m = 0;
    monster_target_y = this->player_ptr->y;
    monster_target_x = this->player_ptr->x;
    auto &floor = *this->player_ptr->current_floor_ptr;

    ProjectResult res;

    auto x1 = this->player_ptr->x;
    auto y1 = this->player_ptr->y;

    auto y2 = target_y;
    auto x2 = target_x;

    if ((x1 == x2) && (y1 == y2)) {
        reset_bits(flag, PROJECT_THRU);
    }

    /* Calculate the projection path */
    handle_stuff(this->player_ptr);

    project_m_n = 0;
    project_m_x = 0;
    project_m_y = 0;
    auto visual = false;
    auto &tracker = LoreTracker::get_instance();
    const auto max_range = AngbandSystem::get_instance().get_max_range();
    while (true) {
        ProjectionPath path_g(this->player_ptr, (project_length ? project_length : max_range), { y1, x1 }, { y2, x2 }, flag);

        if (path_g.path_num() == 0) {
            break;
        }

        for (auto path_g_ite = path_g.begin(); path_g_ite != path_g.end(); path_g_ite++) {
            const auto &[oy, ox] = *(path_g_ite == path_g.begin() ? path_g.begin() : path_g_ite - 1);
            const auto &[ny, nx] = *path_g_ite;

            if (delay_factor > 0 && !this->player_ptr->effects()->blindness().is_blind()) {
                if (panel_contains(ny, nx) && floor.has_los({ ny, nx })) {
                    print_bolt_pict(this->player_ptr, oy, ox, ny, nx, typ);
                    move_cursor_relative(ny, nx);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                    lite_spot(this->player_ptr, ny, nx);
                    term_fresh();

                    print_bolt_pict(this->player_ptr, ny, nx, ny, nx, typ);

                    visual = true;
                } else if (visual) {
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                }
            }

            if (affect_item(this->player_ptr, 0, 0, ny, nx, dam, typ)) {
                res.notice = true;
            }
        }

        for (const auto &[py, px] : path_g) {
            if (affect_monster(this->player_ptr, 0, 0, py, px, dam, typ, flag, true)) {
                res.notice = true;
            }
            const auto &grid = floor.grid_array[project_m_y][project_m_x];
            const auto &monster = floor.m_list[grid.m_idx];
            if (project_m_n == 1 && grid.has_monster() && monster.ml) {
                if (!this->player_ptr->effects()->hallucination().is_hallucinated()) {
                    tracker.set_trackee(monster.ap_r_idx);
                }

                health_track(this->player_ptr, grid.m_idx);
            }

            (void)affect_feature(this->player_ptr, 0, 0, py, px, dam, typ);
        }

        const auto &[y, x] = path_g.back();

        if (!floor.get_grid({ y, x }).is_mirror()) {
            break;
        }

        monster_target_y = y;
        monster_target_x = x;
        this->remove_mirror(y, x);
        const auto pos = this->get_next_mirror_position({ y, x });
        y2 = pos.y;
        x2 = pos.x;
        y1 = y;
        x1 = x;
    }
}

static void draw_super_ray_pict(PlayerType *player_ptr, const std::map<int, std::vector<ProjectionPath::pp_const_iterator>> &pos_list_map,
    const std::vector<ProjectionPath> &second_path_g_list, const Pos2D &center)
{
    if (delay_factor <= 0) {
        return;
    }

    constexpr auto typ = AttributeType::SUPER_RAY;

    // 8方向のスーパーレイの軌道上の最後の到達点の座標のリスト
    std::vector<Pos2D> last_pos_list;
    std::transform(second_path_g_list.begin(), second_path_g_list.end(), std::back_inserter(last_pos_list),
        [](const auto &path_g) { return path_g.back(); });

    // スーパーレイの描画を行った座標のリスト。スーパーレイの全ての描画完了後に描画を消去するのに使用する。
    std::vector<Pos2D> drawn_pos_list;

    const auto &floor = *player_ptr->current_floor_ptr;
    for (const auto &[n, pos_list] : pos_list_map) {
        // スーパーレイの最終到達点の座標の描画を行った座標のリスト。最終到達点の描画を '*' で上書きするのに使用する。
        std::vector<Pos2D> drawn_last_pos_list;

        for (const auto &it : pos_list) {
            const auto &pos = (n == 1) ? center : *std::next(it, -1);
            const auto &pos_new = *it;

            if (panel_contains(pos.y, pos.x) && floor.has_los(pos)) {
                print_bolt_pict(player_ptr, pos.y, pos.x, pos.y, pos.x, typ);
                drawn_pos_list.push_back(pos);
            }
            if (panel_contains(pos_new.y, pos_new.x) && floor.has_los(pos_new)) {
                print_bolt_pict(player_ptr, pos.y, pos.x, pos_new.y, pos_new.x, typ);
                if (std::find(last_pos_list.begin(), last_pos_list.end(), *it) != last_pos_list.end()) {
                    drawn_last_pos_list.push_back(pos_new);
                }
            }
        }
        term_fresh();
        term_xtra(TERM_XTRA_DELAY, delay_factor);

        for (const auto &pos : drawn_last_pos_list) {
            if (panel_contains(pos.y, pos.x) && floor.has_los(pos)) {
                print_bolt_pict(player_ptr, pos.y, pos.x, pos.y, pos.x, typ);
                drawn_pos_list.push_back(pos);
            }
        }
    }

    term_fresh();
    term_xtra(TERM_XTRA_DELAY, delay_factor);

    for (const auto &pos : drawn_pos_list) {
        lite_spot(player_ptr, pos.y, pos.x);
    }
}

static bool activate_super_ray_effect(PlayerType *player_ptr, int y, int x, int dam, BIT_FLAGS flag)
{
    constexpr auto typ = AttributeType::SUPER_RAY;
    auto notice = false;

    (void)affect_feature(player_ptr, 0, 0, y, x, dam, typ);

    if (affect_item(player_ptr, 0, 0, y, x, dam, typ)) {
        notice = true;
    }

    (void)affect_monster(player_ptr, 0, 0, y, x, dam, typ, flag, true);

    const auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto *g_ptr = &floor_ptr->grid_array[project_m_y][project_m_x];
    const auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    if (project_m_n == 1 && g_ptr->has_monster() && m_ptr->ml) {
        if (!player_ptr->effects()->hallucination().is_hallucinated()) {
            LoreTracker::get_instance().set_trackee(m_ptr->ap_r_idx);
        }

        health_track(player_ptr, g_ptr->m_idx);
    }

    return notice;
}

void SpellsMirrorMaster::project_super_ray(int target_x, int target_y, int dam)
{
    POSITION y1;
    POSITION x1;
    POSITION y2;
    POSITION x2;
    constexpr auto typ = AttributeType::SUPER_RAY;
    BIT_FLAGS flag = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_THRU | PROJECT_MIRROR;
    rakubadam_m = 0;
    monster_target_y = this->player_ptr->y;
    monster_target_x = this->player_ptr->x;
    const auto &floor = *this->player_ptr->current_floor_ptr;

    ProjectResult res;

    x1 = this->player_ptr->x;
    y1 = this->player_ptr->y;

    y2 = target_y;
    x2 = target_x;

    if ((x1 == x2) && (y1 == y2)) {
        flag &= ~(PROJECT_THRU);
    }

    /* Calculate the projection path */
    const auto &system = AngbandSystem::get_instance();
    ProjectionPath path_g(this->player_ptr, (project_length ? project_length : system.get_max_range()), { y1, x1 }, { y2, x2 }, flag);
    std::vector<ProjectionPath> second_path_g_list;
    handle_stuff(this->player_ptr);

    if (path_g.path_num() == 0) {
        return;
    }

    project_m_n = 0;
    project_m_x = 0;
    project_m_y = 0;
    auto oy = y1;
    auto ox = x1;
    auto visual = false;
    std::vector<std::pair<int, int>> drawn_pos_list;
    for (const auto &[ny, nx] : path_g) {
        if (delay_factor > 0) {
            if (panel_contains(ny, nx) && floor.has_los({ ny, nx })) {
                print_bolt_pict(this->player_ptr, oy, ox, ny, nx, typ);
                move_cursor_relative(ny, nx);
                term_fresh();
                term_xtra(TERM_XTRA_DELAY, delay_factor);
                lite_spot(this->player_ptr, ny, nx);
                term_fresh();

                print_bolt_pict(this->player_ptr, ny, nx, ny, nx, typ);

                drawn_pos_list.emplace_back(ny, nx);
                visual = true;
            } else if (visual) {
                term_xtra(TERM_XTRA_DELAY, delay_factor);
            }
        }

        if (!cave_has_flag_bold(&floor, ny, nx, TerrainCharacteristics::PROJECT)) {
            break;
        }
        oy = ny;
        ox = nx;
    }

    for (const auto &[y, x] : drawn_pos_list) {
        lite_spot(this->player_ptr, y, x);
    }

    {
        const auto &[y, x] = path_g.back();
        if (floor.get_grid({ y, x }).is_mirror()) {
            this->remove_mirror(y, x);
            auto project_flag = flag;
            reset_bits(project_flag, PROJECT_MIRROR);

            const auto length = project_length ? project_length : system.get_max_range();
            for (const auto &dd : CCW_DD) {
                const Pos2D pos(y, x);
                second_path_g_list.emplace_back(this->player_ptr, length, pos, pos + dd, project_flag);
            }
        }
    }

    for (const auto &[py, px] : path_g) {
        res.notice |= activate_super_ray_effect(this->player_ptr, py, px, dam, flag);
    }

    // 起点の鏡からの距離 → 8方向へのスーパーレイの軌道上のその距離にある座標のイテレータのリストの map
    std::map<int, std::vector<ProjectionPath::pp_const_iterator>> pos_list_map;
    for (const auto &second_path_g : second_path_g_list) {
        for (auto it = second_path_g.begin(); it != second_path_g.end(); ++it) {
            const auto &[o_y, o_x] = path_g.back();
            const auto &[y, x] = *it;
            auto d = distance(o_y, o_x, y, x);
            pos_list_map[d].push_back(it);
        }
    }

    draw_super_ray_pict(this->player_ptr, pos_list_map, second_path_g_list, path_g.back());

    for (auto &&[n, pos_list] : pos_list_map) {
        rand_shuffle(pos_list.begin(), pos_list.end());

        for (const auto &it : pos_list) {
            const auto &[y, x] = *it;
            res.notice |= activate_super_ray_effect(player_ptr, y, x, dam, flag);
        }
    }
}
