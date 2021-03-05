﻿#include "spell-realm/spells-chaos.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 虚無招来処理 /
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * Sorry, it becomes not (void)...
 */
void call_the_void(player_type *caster_ptr)
{
    grid_type *g_ptr;
    bool do_call = TRUE;
    for (int i = 0; i < 9; i++) {
        g_ptr = &caster_ptr->current_floor_ptr->grid_array[caster_ptr->y + ddy_ddd[i]][caster_ptr->x + ddx_ddd[i]];

        if (!cave_has_flag_grid(g_ptr, FF_PROJECT)) {
            if (!g_ptr->mimic || !has_flag(f_info[g_ptr->mimic].flags, FF_PROJECT) || !permanent_wall(&f_info[g_ptr->feat])) {
                do_call = FALSE;
                break;
            }
        }
    }

    if (do_call) {
        for (int i = 1; i < 10; i++) {
            if (i - 5)
                fire_ball(caster_ptr, GF_ROCKET, i, 175, 2);
        }

        for (int i = 1; i < 10; i++) {
            if (i - 5)
                fire_ball(caster_ptr, GF_MANA, i, 175, 3);
        }

        for (int i = 1; i < 10; i++) {
            if (i - 5)
                fire_ball(caster_ptr, GF_NUKE, i, 175, 4);
        }

        return;
    }

    bool is_special_fllor = caster_ptr->current_floor_ptr->inside_quest && is_fixed_quest_idx(caster_ptr->current_floor_ptr->inside_quest);
    is_special_fllor |= !caster_ptr->current_floor_ptr->dun_level;
    if (is_special_fllor) {
        msg_print(_("地面が揺れた。", "The ground trembles."));
        return;
    }

#ifdef JP
    msg_format("あなたは%sを壁に近すぎる場所で唱えてしまった！", ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "祈り" : "呪文"));
#else
    msg_format("You %s the %s too close to a wall!", ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "recite" : "cast"),
        ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "prayer" : "spell"));
#endif
    msg_print(_("大きな爆発音があった！", "There is a loud explosion!"));

    if (one_in_(666)) {
        if (!vanish_dungeon(caster_ptr))
            msg_print(_("ダンジョンは一瞬静まり返った。", "The dungeon becomes quiet for a moment."));
        take_hit(caster_ptr, DAMAGE_NOESCAPE, 100 + randint1(150), _("自殺的な虚無招来", "a suicidal Call the Void"), -1);
        return;
    }

    if (destroy_area(caster_ptr, caster_ptr->y, caster_ptr->x, 15 + caster_ptr->lev + randint0(11), FALSE))
        msg_print(_("ダンジョンが崩壊した...", "The dungeon collapses..."));
    else
        msg_print(_("ダンジョンは大きく揺れた。", "The dungeon trembles."));
    take_hit(caster_ptr, DAMAGE_NOESCAPE, 100 + randint1(150), _("自殺的な虚無招来", "a suicidal Call the Void"), -1);
}

/*!
 * @brief 虚無招来によるフロア中の全壁除去処理 /
 * Vanish all walls in this floor
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @params caster_ptr 術者の参照ポインタ
 * @return 実際に処理が反映された場合TRUE
 */
bool vanish_dungeon(player_type *caster_ptr)
{
    bool is_special_floor = caster_ptr->current_floor_ptr->inside_quest && is_fixed_quest_idx(caster_ptr->current_floor_ptr->inside_quest);
    is_special_floor |= !caster_ptr->current_floor_ptr->dun_level;
    if (is_special_floor)
        return FALSE;

    grid_type *g_ptr;
    feature_type *f_ptr;
    monster_type *m_ptr;
    GAME_TEXT m_name[MAX_NLEN];
    for (POSITION y = 1; y < caster_ptr->current_floor_ptr->height - 1; y++) {
        for (POSITION x = 1; x < caster_ptr->current_floor_ptr->width - 1; x++) {
            g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

            f_ptr = &f_info[g_ptr->feat];
            g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);
            m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
            if (g_ptr->m_idx && monster_csleep_remaining(m_ptr)) {
                (void)set_monster_csleep(caster_ptr, g_ptr->m_idx, 0);
                if (m_ptr->ml) {
                    monster_desc(caster_ptr, m_name, m_ptr, 0);
                    msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
                }
            }

            if (has_flag(f_ptr->flags, FF_HURT_DISI))
                cave_alter_feat(caster_ptr, y, x, FF_HURT_DISI);
        }
    }

    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        g_ptr = &caster_ptr->current_floor_ptr->grid_array[0][x];
        f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && has_flag(f_ptr->flags, FF_HURT_DISI)) {
            g_ptr->mimic = feat_state(caster_ptr, g_ptr->mimic, FF_HURT_DISI);
            if (!has_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }

        g_ptr = &caster_ptr->current_floor_ptr->grid_array[caster_ptr->current_floor_ptr->height - 1][x];
        f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && has_flag(f_ptr->flags, FF_HURT_DISI)) {
            g_ptr->mimic = feat_state(caster_ptr, g_ptr->mimic, FF_HURT_DISI);
            if (!has_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }
    }

    /* Special boundary walls -- Left and right */
    for (POSITION y = 1; y < (caster_ptr->current_floor_ptr->height - 1); y++) {
        g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][0];
        f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && has_flag(f_ptr->flags, FF_HURT_DISI)) {
            g_ptr->mimic = feat_state(caster_ptr, g_ptr->mimic, FF_HURT_DISI);
            if (!has_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }

        g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][caster_ptr->current_floor_ptr->width - 1];
        f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && has_flag(f_ptr->flags, FF_HURT_DISI)) {
            g_ptr->mimic = feat_state(caster_ptr, g_ptr->mimic, FF_HURT_DISI);
            if (!has_flag(f_info[g_ptr->mimic].flags, FF_REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }
    }

    caster_ptr->update |= (PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);
    caster_ptr->redraw |= (PR_MAP);
    caster_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
    return TRUE;
}

/*!
 * @brief カオス魔法「流星群」/トランプ魔法「隕石のカード」の処理としてプレイヤーを中心に隕石落下処理を10+1d10回繰り返す。
 * / Drop 10+1d10 meteor ball at random places near the player
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam ダメージ
 * @param rad 効力の半径
 * @return なし
 * @details このファイルにいるのは、spells-trump.c と比べて行数が少なかったため。それ以上の意図はない
 */
void cast_meteor(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
    int b = 10 + randint1(10);
    for (int i = 0; i < b; i++) {
        POSITION y = 0, x = 0;
        int count;

        for (count = 0; count <= 20; count++) {
            int dy, dx, d;

            x = caster_ptr->x - 8 + randint0(17);
            y = caster_ptr->y - 8 + randint0(17);
            dx = (caster_ptr->x > x) ? (caster_ptr->x - x) : (x - caster_ptr->x);
            dy = (caster_ptr->y > y) ? (caster_ptr->y - y) : (y - caster_ptr->y);
            d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

            if (d >= 9)
                continue;

            floor_type *floor_ptr = caster_ptr->current_floor_ptr;
            if (!in_bounds(floor_ptr, y, x) || !projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)
                || !cave_has_flag_bold(floor_ptr, y, x, FF_PROJECT))
                continue;

            break;
        }

        if (count > 20)
            continue;

        project(caster_ptr, 0, rad, y, x, dam, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
    }
}
