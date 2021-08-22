#include "mind/mind-mirror-master.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-monster.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-numbers.h"
#include "pet/pet-util.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spell-types.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/sight-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/projection-path-calculator.h"
#include "target/target-getter.h"
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
        return false;
    }

    if (!creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].is_mirror()) {
        msg_print(_("鏡の上でないと集中できない！", "There's no mirror here!"));
        return true;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

    creature_ptr->csp += (5 + creature_ptr->lev * creature_ptr->lev / 100);
    if (creature_ptr->csp >= creature_ptr->msp) {
        creature_ptr->csp = creature_ptr->msp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= PR_MANA;
    return true;
}

/*!
 * @brief 全鏡の消去 / Remove all mirrors in this floor
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param explode 爆発処理を伴うならばTRUE
 */
void remove_all_mirrors(player_type *caster_ptr, bool explode)
{
    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++) {
            if (!caster_ptr->current_floor_ptr->grid_array[y][x].is_mirror())
                continue;

            remove_mirror(caster_ptr, y, x);
            if (!explode)
                continue;

            project(caster_ptr, 0, 2, y, x, caster_ptr->lev / 2 + 5, GF_SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI));
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
            if (caster_ptr->current_floor_ptr->grid_array[y][x].is_mirror() && distance(caster_ptr->y, caster_ptr->x, y, x) <= get_max_range(caster_ptr)
                && distance(caster_ptr->y, caster_ptr->x, y, x) != 0 && player_has_los_bold(caster_ptr, y, x)
                && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x)) {
                mirror_y[mirror_num] = y;
                mirror_x[mirror_num] = x;
                mirror_num++;
            }
        }
    }

    if (mirror_num < 2)
        return false;

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
        return false;

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
                        uint16_t p = bolt_pict(y, x, y, x, GF_MANA);
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
                    (void)affect_monster(caster_ptr, 0, 0, y, x, dam, GF_MANA, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), true);
                }
            }
        }
    }

    if (one_in_(7)) {
        msg_print(_("鏡が結界に耐えきれず、壊れてしまった。", "The field broke a mirror"));
        remove_mirror(caster_ptr, point_y[0], point_x[0]);
    }

    return true;
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
            if (!caster_ptr->current_floor_ptr->grid_array[y][x].is_mirror())
                continue;

            if (!affect_monster(caster_ptr, 0, 0, y, x, dam, GF_GENOCIDE, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), true))
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
    msg_print(_("辺りを睨んだ...", "You glare at nearby monsters..."));
    slow_monsters(creature_ptr, creature_ptr->lev);
    stun_monsters(creature_ptr, creature_ptr->lev * 4);
    confuse_monsters(creature_ptr, creature_ptr->lev * 4);
    turn_monsters(creature_ptr, creature_ptr->lev * 4);
    stasis_monsters(creature_ptr, creature_ptr->lev * 4);
    return true;
}

/*!
 * @brief 鏡設置処理
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool place_mirror(player_type *caster_ptr)
{
    if (!cave_clean_bold(caster_ptr->current_floor_ptr, caster_ptr->y, caster_ptr->x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return false;
    }

    /* Create a mirror */
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info |= CAVE_OBJECT;
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].mimic = feat_mirror;

    /* Turn on the light */
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info |= CAVE_GLOW;

    note_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    lite_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    update_local_illumination(caster_ptr, caster_ptr->y, caster_ptr->x);

    return true;
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
        return false;
    if (exe_dimension_door(caster_ptr, x, y))
        return true;

    msg_print(_("鏡の世界をうまく通れなかった！", "You could not enter the mirror!"));
    return true;
}

/*
 * Set "multishadow", notice observable changes
 */
bool set_multishadow(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->multishadow && !do_dec) {
            if (creature_ptr->multishadow > v)
                return false;
        } else if (!creature_ptr->multishadow) {
            msg_print(_("あなたの周りに幻影が生まれた。", "Your Shadow enveloped you."));
            notice = true;
        }
    } else {
        if (creature_ptr->multishadow) {
            msg_print(_("幻影が消えた。", "Your Shadow disappears."));
            notice = true;
        }
    }

    creature_ptr->multishadow = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 一時的破片のオーラの継続時間をセットする / Set "dustrobe", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_dustrobe(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return false;

    if (v) {
        if (creature_ptr->dustrobe && !do_dec) {
            if (creature_ptr->dustrobe > v)
                return false;
        } else if (!creature_ptr->dustrobe) {
            msg_print(_("体が鏡のオーラで覆われた。", "You are enveloped by mirror shards."));
            notice = true;
        }
    } else {
        if (creature_ptr->dustrobe) {
            msg_print(_("鏡のオーラが消えた。", "The mirror shards disappear."));
            notice = true;
        }
    }

    creature_ptr->dustrobe = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(creature_ptr, false, false);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

/*!
 * @brief 現在フロアに存在している鏡の数を数える / calculate mirrors
 * @return 鏡の枚数
 */
static int number_of_mirrors(floor_type *floor_ptr)
{
    int val = 0;
    for (POSITION x = 0; x < floor_ptr->width; x++) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            if (floor_ptr->grid_array[y][x].is_mirror())
                val++;
        }
    }

    return val;
}

/*!
 * @brief 鏡魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'Mirror magic'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_mirror_spell(player_type *caster_ptr, mind_mirror_master_type spell)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;
    int tmp;
    TIME_EFFECT t;
    POSITION x, y;
    auto *g_ptr = &caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x];
    switch (spell) {
    case MIRROR_SEEING:
        tmp = g_ptr->is_mirror() ? 4 : 0;
        if (plev + tmp > 4)
            detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
        if (plev + tmp > 18)
            detect_monsters_invis(caster_ptr, DETECT_RAD_DEFAULT);
        if (plev + tmp > 28)
            set_tim_esp(caster_ptr, (TIME_EFFECT)plev, false);
        if (plev + tmp > 38)
            map_area(caster_ptr, DETECT_RAD_MAP);
        if (tmp == 0 && plev < 5) {
            msg_print(_("鏡がなくて集中できなかった！", "You need a mirror to concentrate!"));
        }
        break;
    case MAKE_MIRROR:
        if (number_of_mirrors(caster_ptr->current_floor_ptr) < 4 + plev / 10)
            place_mirror(caster_ptr);
        else
            msg_format(_("これ以上鏡は制御できない！", "There are too many mirrors to control!"));

        break;
    case DRIP_LIGHT:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        if (plev > 9 && g_ptr->is_mirror())
            fire_beam(caster_ptr, GF_LITE, dir, damroll(3 + ((plev - 1) / 5), 4));
        else
            fire_bolt(caster_ptr, GF_LITE, dir, damroll(3 + ((plev - 1) / 5), 4));

        break;
    case WRAPPED_MIRROR:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case MIRROR_LIGHT:
        (void)lite_area(caster_ptr, damroll(2, (plev / 2)), (plev / 10) + 1);
        break;
    case WANDERING_MIRROR:
        teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case ROBE_DUST:
        set_dustrobe(caster_ptr, 20 + randint1(20), false);
        break;
    case BANISHING_MIRROR:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        (void)fire_beam(caster_ptr, GF_AWAY_ALL, dir, plev);
        break;
    case MIRROR_CRASHING:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        fire_ball(caster_ptr, GF_SHARDS, dir, damroll(8 + ((plev - 5) / 4), 8), (plev > 20 ? (plev - 20) / 8 + 1 : 0));
        break;
    case SLEEPING_MIRROR:
        for (x = 0; x < caster_ptr->current_floor_ptr->width; x++)
            for (y = 0; y < caster_ptr->current_floor_ptr->height; y++)
                if (caster_ptr->current_floor_ptr->grid_array[y][x].is_mirror())
                    project(caster_ptr, 0, 2, y, x, (HIT_POINT)plev, GF_OLD_SLEEP,
                        (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI));

        break;
    case SEEKER_RAY:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        fire_beam(caster_ptr, GF_SEEKER, dir, damroll(11 + (plev - 5) / 4, 8));
        break;
    case SEALING_MIRROR:
        seal_of_mirror(caster_ptr, plev * 4 + 100);
        break;
    case WATER_SHIELD:
        t = 20 + randint1(20);
        set_shield(caster_ptr, t, false);
        if (plev > 31)
            set_tim_reflect(caster_ptr, t, false);

        if (plev > 39)
            set_resist_magic(caster_ptr, t, false);

        break;
    case SUPER_RAY:
        if (!get_aim_dir(caster_ptr, &dir))
            return false;

        fire_beam(caster_ptr, GF_SUPER_RAY, dir, 150 + randint1(2 * plev));
        break;
    case ILLUSION_LIGHT:
        tmp = g_ptr->is_mirror() ? 4 : 3;
        slow_monsters(caster_ptr, plev);
        stun_monsters(caster_ptr, plev * tmp * 2);
        confuse_monsters(caster_ptr, plev * tmp);
        turn_monsters(caster_ptr, plev * tmp);
        stasis_monsters(caster_ptr, plev * tmp);
        break;
    case MIRROR_SHIFT:
        if (!g_ptr->is_mirror()) {
            msg_print(_("鏡の国の場所がわからない！", "You cannot find out where the mirror is!"));
            break;
        }

        reserve_alter_reality(caster_ptr, randint0(21) + 15);
        break;
    case MIRROR_TUNNEL:
        msg_print(_("鏡の世界を通り抜け…  ", "You try to enter the mirror..."));
        return mirror_tunnel(caster_ptr);
    case RECALL_MIRROR:
        return recall_player(caster_ptr, randint0(21) + 15);
    case MULTI_SHADOW:
        set_multishadow(caster_ptr, 6 + randint1(6), false);
        break;
    case BINDING_FIELD:
        if (!binding_field(caster_ptr, plev * 11 + 5))
            msg_print(_("適当な鏡を選べなかった！", "You were not able to choose suitable mirrors!"));

        break;
    case RUFFNOR_MIRROR:
        (void)set_invuln(caster_ptr, randint1(4) + 4, false);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }

    caster_ptr->magic_num1[0] = 0;
    return true;
}
