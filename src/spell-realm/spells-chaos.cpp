#include "spell-realm/spells-chaos.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "player-info/class-info.h"
#include "player/player-damage.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 虚無招来処理 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Sorry, it becomes not (void)...
 */
void call_the_void(player_type *player_ptr)
{
    bool do_call = true;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < 9; i++) {
        auto *g_ptr = &floor_ptr->grid_array[player_ptr->y + ddy_ddd[i]][player_ptr->x + ddx_ddd[i]];

        if (!g_ptr->cave_has_flag(FF::PROJECT)) {
            if (!g_ptr->mimic || f_info[g_ptr->mimic].flags.has_not(FF::PROJECT) || !permanent_wall(&f_info[g_ptr->feat])) {
                do_call = false;
                break;
            }
        }
    }

    if (do_call) {
        for (int i = 1; i < 10; i++) {
            if (i - 5)
                fire_ball(player_ptr, GF_ROCKET, i, 175, 2);
        }

        for (int i = 1; i < 10; i++) {
            if (i - 5)
                fire_ball(player_ptr, GF_MANA, i, 175, 3);
        }

        for (int i = 1; i < 10; i++) {
            if (i - 5)
                fire_ball(player_ptr, GF_NUKE, i, 175, 4);
        }

        return;
    }

    bool is_special_fllor = floor_ptr->inside_quest && quest_type::is_fixed(floor_ptr->inside_quest);
    is_special_fllor |= floor_ptr->dun_level > 0;
    if (is_special_fllor) {
        msg_print(_("地面が揺れた。", "The ground trembles."));
        return;
    }

#ifdef JP
    msg_format("あなたは%sを壁に近すぎる場所で唱えてしまった！", ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "祈り" : "呪文"));
#else
    msg_format("You %s the %s too close to a wall!", ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "recite" : "cast"),
        ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "prayer" : "spell"));
#endif
    msg_print(_("大きな爆発音があった！", "There is a loud explosion!"));

    if (one_in_(666)) {
        if (!vanish_dungeon(player_ptr))
            msg_print(_("ダンジョンは一瞬静まり返った。", "The dungeon becomes quiet for a moment."));
        take_hit(player_ptr, DAMAGE_NOESCAPE, 100 + randint1(150), _("自殺的な虚無招来", "a suicidal Call the Void"));
        return;
    }

    if (destroy_area(player_ptr, player_ptr->y, player_ptr->x, 15 + player_ptr->lev + randint0(11), false))
        msg_print(_("ダンジョンが崩壊した...", "The dungeon collapses..."));
    else
        msg_print(_("ダンジョンは大きく揺れた。", "The dungeon trembles."));
    take_hit(player_ptr, DAMAGE_NOESCAPE, 100 + randint1(150), _("自殺的な虚無招来", "a suicidal Call the Void"));
}

/*!
 * @brief 虚無招来によるフロア中の全壁除去処理 /
 * Vanish all walls in this floor
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param player_ptr 術者の参照ポインタ
 * @return 実際に処理が反映された場合TRUE
 */
bool vanish_dungeon(player_type *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    bool is_special_floor = floor_ptr->inside_quest && quest_type::is_fixed(floor_ptr->inside_quest);
    is_special_floor |= floor_ptr->dun_level > 0;
    if (is_special_floor)
        return false;

    GAME_TEXT m_name[MAX_NLEN];
    for (POSITION y = 1; y < floor_ptr->height - 1; y++) {
        for (POSITION x = 1; x < floor_ptr->width - 1; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];

            auto *f_ptr = &f_info[g_ptr->feat];
            g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);
            auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
            if (g_ptr->m_idx && monster_csleep_remaining(m_ptr)) {
                (void)set_monster_csleep(player_ptr, g_ptr->m_idx, 0);
                if (m_ptr->ml) {
                    monster_desc(player_ptr, m_name, m_ptr, 0);
                    msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
                }
            }

            if (f_ptr->flags.has(FF::HURT_DISI))
                cave_alter_feat(player_ptr, y, x, FF::HURT_DISI);
        }
    }

    for (POSITION x = 0; x < floor_ptr->width; x++) {
        auto *g_ptr = &floor_ptr->grid_array[0][x];
        auto *f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && f_ptr->flags.has(FF::HURT_DISI)) {
            g_ptr->mimic = feat_state(floor_ptr, g_ptr->mimic, FF::HURT_DISI);
            if (f_info[g_ptr->mimic].flags.has_not(FF::REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }

        g_ptr = &floor_ptr->grid_array[floor_ptr->height - 1][x];
        f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && f_ptr->flags.has(FF::HURT_DISI)) {
            g_ptr->mimic = feat_state(floor_ptr, g_ptr->mimic, FF::HURT_DISI);
            if (f_info[g_ptr->mimic].flags.has_not(FF::REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }
    }

    /* Special boundary walls -- Left and right */
    for (POSITION y = 1; y < (floor_ptr->height - 1); y++) {
        auto *g_ptr = &floor_ptr->grid_array[y][0];
        auto *f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && f_ptr->flags.has(FF::HURT_DISI)) {
            g_ptr->mimic = feat_state(floor_ptr, g_ptr->mimic, FF::HURT_DISI);
            if (f_info[g_ptr->mimic].flags.has_not(FF::REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }

        g_ptr = &floor_ptr->grid_array[y][floor_ptr->width - 1];
        f_ptr = &f_info[g_ptr->mimic];
        g_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

        if (g_ptr->mimic && f_ptr->flags.has(FF::HURT_DISI)) {
            g_ptr->mimic = feat_state(floor_ptr, g_ptr->mimic, FF::HURT_DISI);
            if (f_info[g_ptr->mimic].flags.has_not(FF::REMEMBER))
                g_ptr->info &= ~(CAVE_MARK);
        }
    }

    player_ptr->update |= (PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);
    player_ptr->redraw |= (PR_MAP);
    player_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);
    return true;
}

/*!
 * @brief カオス魔法「流星群」/トランプ魔法「隕石のカード」の処理としてプレイヤーを中心に隕石落下処理を10+1d10回繰り返す。
 * / Drop 10+1d10 meteor ball at random places near the player
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam ダメージ
 * @param rad 効力の半径
 * @details このファイルにいるのは、spells-trump.c と比べて行数が少なかったため。それ以上の意図はない
 */
void cast_meteor(player_type *player_ptr, HIT_POINT dam, POSITION rad)
{
    int b = 10 + randint1(10);
    for (int i = 0; i < b; i++) {
        POSITION y = 0, x = 0;
        int count;

        for (count = 0; count <= 20; count++) {
            int dy, dx, d;

            x = player_ptr->x - 8 + randint0(17);
            y = player_ptr->y - 8 + randint0(17);
            dx = (player_ptr->x > x) ? (player_ptr->x - x) : (x - player_ptr->x);
            dy = (player_ptr->y > y) ? (player_ptr->y - y) : (y - player_ptr->y);
            d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

            if (d >= 9)
                continue;

            floor_type *floor_ptr = player_ptr->current_floor_ptr;
            if (!in_bounds(floor_ptr, y, x) || !projectable(player_ptr, player_ptr->y, player_ptr->x, y, x)
                || !cave_has_flag_bold(floor_ptr, y, x, FF::PROJECT))
                continue;

            break;
        }

        if (count > 20)
            continue;

        project(player_ptr, 0, rad, y, x, dam, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM);
    }
}
