/*!
 * @brief 鏡使いの鏡魔法効果処理
 * @date 2022/03/07
 * @author Hourier
 * @todo 作りかけの部分複数あり
 */

#include "spell-class/spells-mirror-master.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
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
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "term/gameterm.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

SpellsMirrorMaster::SpellsMirrorMaster(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

void SpellsMirrorMaster::remove_mirror(int y, int x)
{
    auto *g_ptr = &this->player_ptr->current_floor_ptr->grid_array[y][x];
    reset_bits(g_ptr->info, CAVE_OBJECT);
    g_ptr->mimic = 0;
    if (d_info[this->player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
        reset_bits(g_ptr->info, CAVE_GLOW);
        if (!view_torch_grids) {
            reset_bits(g_ptr->info, CAVE_MARK);
        }

        if (g_ptr->m_idx) {
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

    set_bits(this->player_ptr->redraw, PR_MANA);
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

            if (g_ref.m_idx == 0) {
                this->remove_mirror(y, x);
            }
        }
    }
}

void SpellsMirrorMaster::seeker_ray(int dir, int dam)
{
    POSITION tx = this->player_ptr->x + ddx[dir];
    POSITION ty = this->player_ptr->y + ddy[dir];
    if ((dir == 5) && target_okay(this->player_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    project_seeker_ray(tx, ty, dam);
}

void SpellsMirrorMaster::super_ray(int dir, int dam)
{
    POSITION tx = this->player_ptr->x + ddx[dir];
    POSITION ty = this->player_ptr->y + ddy[dir];
    if ((dir == 5) && target_okay(this->player_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    project_super_ray(tx, ty, dam);
}

/*!
 * @brief 配置した鏡リストの次を取得する /
 * Get another mirror. for SEEKER
 * @param next_y 次の鏡のy座標を返す参照ポインタ
 * @param next_x 次の鏡のx座標を返す参照ポインタ
 * @param cury 現在の鏡のy座標
 * @param curx 現在の鏡のx座標
 */
void SpellsMirrorMaster::next_mirror(int *next_y, int *next_x, int cury, int curx)
{
    POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
    int mirror_num = 0; /* 鏡の数 */

    auto *floor_ptr = this->player_ptr->current_floor_ptr;

    for (POSITION x = 0; x < floor_ptr->width; x++) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            if (floor_ptr->grid_array[y][x].is_mirror()) {
                mirror_y[mirror_num] = y;
                mirror_x[mirror_num] = x;
                mirror_num++;
            }
        }
    }

    if (mirror_num) {
        int num = randint0(mirror_num);
        *next_y = mirror_y[num];
        *next_x = mirror_x[num];
        return;
    }

    *next_y = cury + randint0(5) - 2;
    *next_x = curx + randint0(5) - 2;
}

void SpellsMirrorMaster::project_seeker_ray(int target_x, int target_y, int dam)
{
    POSITION y1;
    POSITION x1;
    POSITION y2;
    POSITION x2;
    bool blind = this->player_ptr->blind != 0;
    auto path_n = 0;
    uint16_t path_g[512];
    constexpr auto typ = AttributeType::SEEKER;
    BIT_FLAGS flag = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_THRU;
    rakubadam_p = 0;
    rakubadam_m = 0;
    monster_target_y = this->player_ptr->y;
    monster_target_x = this->player_ptr->x;

    ProjectResult res;

    x1 = this->player_ptr->x;
    y1 = this->player_ptr->y;

    y2 = target_y;
    x2 = target_x;

    if ((x1 == x2) && (y1 == y2)) {
        reset_bits(flag, PROJECT_THRU);
    }

    /* Calculate the projection path */
    path_n = projection_path(this->player_ptr, path_g, (project_length ? project_length : get_max_range(this->player_ptr)), y1, x1, y2, x2, flag);
    handle_stuff(this->player_ptr);

    auto last_i = 0;
    project_m_n = 0;
    project_m_x = 0;
    project_m_y = 0;
    auto oy = y1;
    auto ox = x1;
    auto visual = false;
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto i = 0; i < path_n; ++i) {
        auto ny = get_grid_y(path_g[i]);
        auto nx = get_grid_x(path_g[i]);

        if (delay_factor > 0) {
            if (!blind) {
                if (panel_contains(ny, nx) && player_has_los_bold(this->player_ptr, ny, nx)) {
                    auto p = bolt_pict(oy, ox, ny, nx, typ);
                    auto a = PICT_A(p);
                    auto c = PICT_C(p);
                    print_rel(this->player_ptr, c, a, ny, nx);
                    move_cursor_relative(ny, nx);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                    lite_spot(this->player_ptr, ny, nx);
                    term_fresh();

                    p = bolt_pict(ny, nx, ny, nx, typ);
                    a = PICT_A(p);
                    c = PICT_C(p);
                    print_rel(this->player_ptr, c, a, ny, nx);

                    visual = true;
                } else if (visual) {
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                }
            }
        }

        if (affect_item(this->player_ptr, 0, 0, ny, nx, dam, typ)) {
            res.notice = true;
        }

        if (!floor_ptr->grid_array[ny][nx].is_mirror()) {
            oy = ny;
            ox = nx;
            continue;
        }

        monster_target_y = ny;
        monster_target_x = nx;
        SpellsMirrorMaster(this->player_ptr).remove_mirror(ny, nx);

        int next_y, next_x;
        this->next_mirror(&next_y, &next_x, ny, nx);

        path_n = i + projection_path(this->player_ptr, &(path_g[i + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), ny, nx, next_y, next_x, flag);
        auto y = ny;
        auto x = nx;
        for (auto j = last_i; j <= i; j++) {
            y = get_grid_y(path_g[j]);
            x = get_grid_x(path_g[j]);
            if (affect_monster(this->player_ptr, 0, 0, y, x, dam, typ, flag, true)) {
                res.notice = true;
            }
            auto *g_ptr = &floor_ptr->grid_array[project_m_y][project_m_x];
            if ((project_m_n == 1) && (g_ptr->m_idx > 0)) {
                auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
                if (m_ptr->ml) {
                    if (!this->player_ptr->effects()->hallucination()->is_hallucinated()) {
                        monster_race_track(this->player_ptr, m_ptr->ap_r_idx);
                    }

                    health_track(this->player_ptr, g_ptr->m_idx);
                }
            }

            (void)affect_feature(this->player_ptr, 0, 0, y, x, dam, typ);
        }

        last_i = i;
        oy = y;
        ox = x;
    }

    for (auto i = last_i; i < path_n; i++) {
        auto py = get_grid_y(path_g[i]);
        auto px = get_grid_x(path_g[i]);
        if (affect_monster(this->player_ptr, 0, 0, py, px, dam, typ, flag, true)) {
            res.notice = true;
        }
        if (project_m_n == 1) {
            auto *g_ptr = &floor_ptr->grid_array[project_m_y][project_m_x];
            if (g_ptr->m_idx > 0) {
                auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
                if (m_ptr->ml) {
                    if (!this->player_ptr->effects()->hallucination()->is_hallucinated()) {
                        monster_race_track(this->player_ptr, m_ptr->ap_r_idx);
                    }
                    health_track(this->player_ptr, g_ptr->m_idx);
                }
            }
        }

        (void)affect_feature(this->player_ptr, 0, 0, py, px, dam, typ);
    }
}

void SpellsMirrorMaster::project_super_ray(int target_x, int target_y, int dam)
{
    POSITION y1;
    POSITION x1;
    POSITION y2;
    POSITION x2;
    int path_n = 0;
    uint16_t path_g[512];
    constexpr auto typ = AttributeType::SUPER_RAY;
    BIT_FLAGS flag = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_THRU;
    rakubadam_p = 0;
    rakubadam_m = 0;
    monster_target_y = this->player_ptr->y;
    monster_target_x = this->player_ptr->x;

    ProjectResult res;

    x1 = this->player_ptr->x;
    y1 = this->player_ptr->y;

    y2 = target_y;
    x2 = target_x;

    if ((x1 == x2) && (y1 == y2)) {
        flag &= ~(PROJECT_THRU);
    }

    /* Calculate the projection path */
    path_n = projection_path(this->player_ptr, path_g, (project_length ? project_length : get_max_range(this->player_ptr)), y1, x1, y2, x2, flag);
    handle_stuff(this->player_ptr);

    auto second_step = 0;
    project_m_n = 0;
    project_m_x = 0;
    project_m_y = 0;
    auto oy = y1;
    auto ox = x1;
    auto visual = false;
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    for (auto i = 0; i < path_n; ++i) {
        auto ny = get_grid_y(path_g[i]);
        auto nx = get_grid_x(path_g[i]);
        {
            if (delay_factor > 0) {
                if (panel_contains(ny, nx) && player_has_los_bold(this->player_ptr, ny, nx)) {
                    auto p = bolt_pict(oy, ox, ny, nx, typ);
                    auto a = PICT_A(p);
                    auto c = PICT_C(p);
                    print_rel(this->player_ptr, c, a, ny, nx);
                    move_cursor_relative(ny, nx);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                    lite_spot(this->player_ptr, ny, nx);
                    term_fresh();

                    p = bolt_pict(ny, nx, ny, nx, typ);
                    a = PICT_A(p);
                    c = PICT_C(p);
                    print_rel(this->player_ptr, c, a, ny, nx);

                    visual = true;
                } else if (visual) {
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                }
            }
        }

        if (affect_item(this->player_ptr, 0, 0, ny, nx, dam, typ)) {
            res.notice = true;
        }
        if (!cave_has_flag_bold(floor_ptr, ny, nx, FloorFeatureType::PROJECT)) {
            if (second_step) {
                continue;
            }
            break;
        }

        if (floor_ptr->grid_array[ny][nx].is_mirror() && !second_step) {
            monster_target_y = ny;
            monster_target_x = nx;
            SpellsMirrorMaster(this->player_ptr).remove_mirror(ny, nx);
            auto y = ny;
            auto x = nx;
            for (auto j = 0; j <= i; j++) {
                y = get_grid_y(path_g[j]);
                x = get_grid_x(path_g[j]);
                (void)affect_feature(this->player_ptr, 0, 0, y, x, dam, typ);
            }

            path_n = i;
            second_step = i + 1;
            path_n += projection_path(
                this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y - 1, x - 1, flag);
            path_n += projection_path(this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y - 1, x, flag);
            path_n += projection_path(
                this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y - 1, x + 1, flag);
            path_n += projection_path(this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y, x - 1, flag);
            path_n += projection_path(this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y, x + 1, flag);
            path_n += projection_path(
                this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y + 1, x - 1, flag);
            path_n += projection_path(this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y + 1, x, flag);
            path_n += projection_path(
                this->player_ptr, &(path_g[path_n + 1]), (project_length ? project_length : get_max_range(this->player_ptr)), y, x, y + 1, x + 1, flag);

            oy = y;
            ox = x;
            continue;
        }
        oy = ny;
        ox = nx;
    }

    for (auto i = 0; i < path_n; i++) {
        auto py = get_grid_y(path_g[i]);
        auto px = get_grid_x(path_g[i]);
        (void)affect_monster(this->player_ptr, 0, 0, py, px, dam, typ, flag, true);
        if (project_m_n == 1) {
            auto *g_ptr = &floor_ptr->grid_array[project_m_y][project_m_x];
            if (g_ptr->m_idx > 0) {
                auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
                if (m_ptr->ml) {
                    if (!this->player_ptr->effects()->hallucination()->is_hallucinated()) {
                        monster_race_track(this->player_ptr, m_ptr->ap_r_idx);
                    }
                    health_track(this->player_ptr, g_ptr->m_idx);
                }
            }
        }

        (void)affect_feature(this->player_ptr, 0, 0, py, px, dam, typ);
    }
}
