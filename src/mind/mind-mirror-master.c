#include "mind/mind-mirror-master.h"
#include "cmd-action/cmd-pet.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-monster.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "io/targeting.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/process-effect.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*
 * @brief Multishadow effects is determined by turn
 */
bool check_multishadow(player_type *creature_ptr) { return (creature_ptr->multishadow != 0) && ((current_world_ptr->game_turn & 1) != 0); }

/*!
 * 静水
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return ペットを操っている場合を除きTRUE
 */
bool mirror_concentration(player_type *creature_ptr)
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return FALSE;
    }

    if (!is_mirror_grid(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])) {
        msg_print(_("鏡の上でないと集中できない！", "There's no mirror here!"));
        return TRUE;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

    creature_ptr->csp += (5 + creature_ptr->lev * creature_ptr->lev / 100);
    if (creature_ptr->csp >= creature_ptr->msp) {
        creature_ptr->csp = creature_ptr->msp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= PR_MANA;
    return TRUE;
}

/*!
 * @brief 全鏡の消去 / Remove all mirrors in this floor
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param explode 爆発処理を伴うならばTRUE
 * @return なし
 */
void remove_all_mirrors(player_type *caster_ptr, bool explode)
{
    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++) {
            if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
                continue;

            remove_mirror(caster_ptr, y, x);
            if (!explode)
                continue;

            project(caster_ptr, 0, 2, y, x, caster_ptr->lev / 2 + 5, GF_SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
        }
    }
}

/*!
 * @brief 鏡魔法「封魔結界」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
bool binding_field(player_type *caster_ptr, HIT_POINT dam)
{
    POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
    int mirror_num = 0; /* 鏡の数 */
    int msec = delay_factor * delay_factor * delay_factor;

    /* 三角形の頂点 */
    POSITION point_x[3];
    POSITION point_y[3];

    /* Default target of monsterspell is player */
    monster_target_y = caster_ptr->y;
    monster_target_x = caster_ptr->x;

    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++) {
            if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]) && distance(caster_ptr->y, caster_ptr->x, y, x) <= get_max_range(caster_ptr)
                && distance(caster_ptr->y, caster_ptr->x, y, x) != 0 && player_has_los_bold(caster_ptr, y, x)
                && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)) {
                mirror_y[mirror_num] = y;
                mirror_x[mirror_num] = x;
                mirror_num++;
            }
        }
    }

    if (mirror_num < 2)
        return FALSE;

    point_x[0] = randint0(mirror_num);
    do {
        point_x[1] = randint0(mirror_num);
    } while (point_x[0] == point_x[1]);

    point_y[0] = mirror_y[point_x[0]];
    point_x[0] = mirror_x[point_x[0]];
    point_y[1] = mirror_y[point_x[1]];
    point_x[1] = mirror_x[point_x[1]];
    point_y[2] = caster_ptr->y;
    point_x[2] = caster_ptr->x;

    POSITION x = point_x[0] + point_x[1] + point_x[2];
    POSITION y = point_y[0] + point_y[1] + point_y[2];

    POSITION centersign = (point_x[0] * 3 - x) * (point_y[1] * 3 - y) - (point_y[0] * 3 - y) * (point_x[1] * 3 - x);
    if (centersign == 0)
        return FALSE;

    POSITION x1 = point_x[0] < point_x[1] ? point_x[0] : point_x[1];
    x1 = x1 < point_x[2] ? x1 : point_x[2];
    POSITION y1 = point_y[0] < point_y[1] ? point_y[0] : point_y[1];
    y1 = y1 < point_y[2] ? y1 : point_y[2];

    POSITION x2 = point_x[0] > point_x[1] ? point_x[0] : point_x[1];
    x2 = x2 > point_x[2] ? x2 : point_x[2];
    POSITION y2 = point_y[0] > point_y[1] ? point_y[0] : point_y[1];
    y2 = y2 > point_y[2] ? y2 : point_y[2];

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) >= 0
                && centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) >= 0
                && centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) >= 0) {
                if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)) {
                    if (!(caster_ptr->blind) && panel_contains(y, x)) {
                        u16b p = bolt_pict(y, x, y, x, GF_MANA);
                        print_rel(caster_ptr, PICT_C(p), PICT_A(p), y, x);
                        move_cursor_relative(y, x);
                        term_fresh();
                        term_xtra(TERM_XTRA_DELAY, msec);
                    }
                }
            }
        }
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) >= 0
                && centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) >= 0
                && centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) >= 0) {
                if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)) {
                    (void)affect_feature(caster_ptr, 0, 0, y, x, dam, GF_MANA);
                }
            }
        }
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) >= 0
                && centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) >= 0
                && centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) >= 0) {
                if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)) {
                    (void)affect_item(caster_ptr, 0, 0, y, x, dam, GF_MANA);
                }
            }
        }
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) >= 0
                && centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) >= 0
                && centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) >= 0) {
                if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)) {
                    (void)affect_monster(caster_ptr, 0, 0, y, x, dam, GF_MANA, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE);
                }
            }
        }
    }

    if (one_in_(7)) {
        msg_print(_("鏡が結界に耐えきれず、壊れてしまった。", "The field broke a mirror"));
        remove_mirror(caster_ptr, point_y[0], point_x[0]);
    }

    return TRUE;
}

/*!
 * @brief 鏡魔法「鏡の封印」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam)
{
    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++) {
            if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
                continue;

            if (!affect_monster(caster_ptr, 0, 0, y, x, dam, GF_GENOCIDE, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE))
                continue;

            if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                remove_mirror(caster_ptr, y, x);
            }
        }
    }
}

/*!
 * 幻惑の光 @ 鏡使いだけでなく混沌の戦士も使える
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool confusing_light(player_type *creature_ptr)
{
    msg_print(_("辺りを睨んだ...", "You glare nearby monsters..."));
    slow_monsters(creature_ptr, creature_ptr->lev);
    stun_monsters(creature_ptr, creature_ptr->lev * 4);
    confuse_monsters(creature_ptr, creature_ptr->lev * 4);
    turn_monsters(creature_ptr, creature_ptr->lev * 4);
    stasis_monsters(creature_ptr, creature_ptr->lev * 4);
    return TRUE;
}

/*!
 * @brief 鏡設置処理
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool place_mirror(player_type *caster_ptr)
{
    if (!cave_clean_bold(caster_ptr->current_floor_ptr, caster_ptr->y, caster_ptr->x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return FALSE;
    }

    /* Create a mirror */
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info |= CAVE_OBJECT;
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].mimic = feat_mirror;

    /* Turn on the light */
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info |= CAVE_GLOW;

    note_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    lite_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    update_local_illumination(caster_ptr, caster_ptr->y, caster_ptr->x);

    return TRUE;
}

/*!
 * @brief 鏡抜け処理のメインルーチン /
 * Mirror Master's Dimension Door
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return ターンを消費した場合TRUEを返す
 */
bool mirror_tunnel(player_type *caster_ptr)
{
    POSITION x = 0, y = 0;
    if (!tgt_pt(caster_ptr, &x, &y))
        return FALSE;
    if (exe_dimension_door(caster_ptr, x, y))
        return TRUE;

    msg_print(_("鏡の世界をうまく通れなかった！", "You fail to pass the mirror plane correctly!"));
    return TRUE;
}

/*
 * Set "multishadow", notice observable changes
 */
bool set_multishadow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->multishadow && !do_dec) {
            if (creature_ptr->multishadow > v)
                return FALSE;
        } else if (!creature_ptr->multishadow) {
            msg_print(_("あなたの周りに幻影が生まれた。", "Your Shadow enveloped you."));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->multishadow) {
            msg_print(_("幻影が消えた。", "Your Shadow disappears."));
            notice = TRUE;
        }
    }

    creature_ptr->multishadow = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 一時的破片のオーラの継続時間をセットする / Set "dustrobe", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_dustrobe(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->dustrobe && !do_dec) {
            if (creature_ptr->dustrobe > v)
                return FALSE;
        } else if (!creature_ptr->dustrobe) {
            msg_print(_("体が鏡のオーラで覆われた。", "You were enveloped by mirror shards."));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->dustrobe) {
            msg_print(_("鏡のオーラが消えた。", "The mirror shards disappear."));
            notice = TRUE;
        }
    }

    creature_ptr->dustrobe = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}
