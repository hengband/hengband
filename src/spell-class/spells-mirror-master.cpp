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
    constexpr auto typ = AttributeType::SEEKER;
    BIT_FLAGS flag = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_THRU | PROJECT_MIRROR;
    rakubadam_p = 0;
    rakubadam_m = 0;
    monster_target_y = this->player_ptr->y;
    monster_target_x = this->player_ptr->x;
    auto *floor_ptr = this->player_ptr->current_floor_ptr;

    ProjectResult res;

    x1 = this->player_ptr->x;
    y1 = this->player_ptr->y;

    y2 = target_y;
    x2 = target_x;

    if ((x1 == x2) && (y1 == y2)) {
        reset_bits(flag, PROJECT_THRU);
    }

    /* Calculate the projection path */
    handle_stuff(this->player_ptr);

    project_m_n = 0;
    project_m_x = 0;
    project_m_y = 0;
    auto visual = false;

    while (true) {
        projection_path path_g(this->player_ptr, (project_length ? project_length : get_max_range(this->player_ptr)), y1, x1, y2, x2, flag);

        if (path_g.path_num() == 0) {
            break;
        }

        for (auto path_g_ite = path_g.begin(); path_g_ite != path_g.end(); path_g_ite++) {
            const auto [oy, ox] = *(path_g_ite == path_g.begin() ? path_g.begin() : path_g_ite - 1);
            const auto [ny, nx] = *path_g_ite;

            if (delay_factor > 0 && !blind) {
                if (panel_contains(ny, nx) && player_has_los_bold(this->player_ptr, ny, nx)) {
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
            auto *g_ptr = &floor_ptr->grid_array[project_m_y][project_m_x];
            auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
            if (project_m_n == 1 && g_ptr->m_idx > 0 && m_ptr->ml) {
                if (!this->player_ptr->effects()->hallucination()->is_hallucinated()) {
                    monster_race_track(this->player_ptr, m_ptr->ap_r_idx);
                }
                health_track(this->player_ptr, g_ptr->m_idx);
            }

            (void)affect_feature(this->player_ptr, 0, 0, py, px, dam, typ);
        }

        const auto [y, x] = path_g.back();

        if (!floor_ptr->grid_array[y][x].is_mirror()) {
            break;
        }

        monster_target_y = y;
        monster_target_x = x;
        this->remove_mirror(y, x);
        this->next_mirror(&y2, &x2, y, x);
        y1 = y;
        x1 = x;
    }
}

void SpellsMirrorMaster::project_super_ray(int target_x, int target_y, int dam)
{
    POSITION y1;
    POSITION x1;
    POSITION y2;
    POSITION x2;
    constexpr auto typ = AttributeType::SUPER_RAY;
    BIT_FLAGS flag = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM | PROJECT_THRU | PROJECT_MIRROR;
    rakubadam_p = 0;
    rakubadam_m = 0;
    monster_target_y = this->player_ptr->y;
    monster_target_x = this->player_ptr->x;
    auto *floor_ptr = this->player_ptr->current_floor_ptr;

    ProjectResult res;

    x1 = this->player_ptr->x;
    y1 = this->player_ptr->y;

    y2 = target_y;
    x2 = target_x;

    if ((x1 == x2) && (y1 == y2)) {
        flag &= ~(PROJECT_THRU);
    }

    /* Calculate the projection path */
    projection_path path_g(this->player_ptr, (project_length ? project_length : get_max_range(this->player_ptr)), y1, x1, y2, x2, flag);
    std::vector<projection_path> second_path_g_list;
    handle_stuff(this->player_ptr);

    project_m_n = 0;
    project_m_x = 0;
    project_m_y = 0;
    auto oy = y1;
    auto ox = x1;
    auto visual = false;
    for (const auto &[ny, nx] : path_g) {
        if (delay_factor > 0) {
            if (panel_contains(ny, nx) && player_has_los_bold(this->player_ptr, ny, nx)) {
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
        if (!cave_has_flag_bold(floor_ptr, ny, nx, FloorFeatureType::PROJECT)) {
            break;
        }
        oy = ny;
        ox = nx;
    }
    {
        const auto [y, x] = path_g.back();
        if (floor_ptr->grid_array[y][x].is_mirror()) {
            this->remove_mirror(y, x);
            auto project_flag = flag;
            reset_bits(project_flag, PROJECT_MIRROR);

            const auto length = project_length ? project_length : get_max_range(this->player_ptr);
            for (auto i : { 7, 8, 9, 4, 6, 1, 2, 3 }) {
                const auto dy = ddy[i];
                const auto dx = ddx[i];
                second_path_g_list.emplace_back(this->player_ptr, length, y, x, y + dy, x + dx, project_flag);
            }
        }
    }
    for (const auto &[py, px] : path_g) {
        (void)affect_monster(this->player_ptr, 0, 0, py, px, dam, typ, flag, true);
        auto *g_ptr = &floor_ptr->grid_array[project_m_y][project_m_x];
        auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
        if (project_m_n == 1 && g_ptr->m_idx > 0 && m_ptr->ml) {
            if (!this->player_ptr->effects()->hallucination()->is_hallucinated()) {
                monster_race_track(this->player_ptr, m_ptr->ap_r_idx);
            }
            health_track(this->player_ptr, g_ptr->m_idx);
        }

        (void)affect_feature(this->player_ptr, 0, 0, py, px, dam, typ);
    }
    for (const auto &second_path_g : second_path_g_list) {
        oy = path_g.back().first;
        ox = path_g.back().second;
        for (const auto &[ny, nx] : second_path_g) {
            if (delay_factor > 0) {
                if (panel_contains(ny, nx) && player_has_los_bold(this->player_ptr, ny, nx)) {
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

            oy = ny;
            ox = nx;
        }
    }
    for (const auto &second_path_g : second_path_g_list) {
        for (const auto &[py, px] : second_path_g) {
            (void)affect_monster(this->player_ptr, 0, 0, py, px, dam, typ, flag, true);
            auto *g_ptr = &floor_ptr->grid_array[project_m_y][project_m_x];
            auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
            if (project_m_n == 1 && g_ptr->m_idx > 0 && m_ptr->ml) {
                if (!this->player_ptr->effects()->hallucination()->is_hallucinated()) {
                    monster_race_track(this->player_ptr, m_ptr->ap_r_idx);
                }
                health_track(this->player_ptr, g_ptr->m_idx);
            }

            (void)affect_feature(this->player_ptr, 0, 0, py, px, dam, typ);
        }
    }
}
