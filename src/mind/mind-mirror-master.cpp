/*!
 * @brief 鏡使いの鏡魔法コマンド処理
 * @date 2022/03/07
 * @author Hourier
 * @todo 作りかけの部分複数あり
 */

#include "mind/mind-mirror-master.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-monster.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-numbers.h"
#include "pet/pet-util.h"
#include "spell-class/spells-mirror-master.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/sight-setter.h"
#include "system/angband-system.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/grid-selector.h"
#include "target/projection-path-calculator.h"
#include "target/target-getter.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "util/point-2d.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <range/v3/algorithm.hpp>

/*
 * @brief Multishadow effects is determined by turn
 */
bool check_multishadow(PlayerType *player_ptr)
{
    return (player_ptr->multishadow != 0) && ((AngbandWorld::get_instance().game_turn & 1) != 0);
}

/*!
 * @brief 鏡魔法「封魔結界」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
bool binding_field(PlayerType *player_ptr, int dam)
{
    /* 鏡はもっと少ない */
    int mirror_x[10]{};
    int mirror_y[10]{};
    int mirror_num = 0; /* 鏡の数 */

    /* 三角形の頂点 */
    int point_x[3]{};
    int point_y[3]{};

    /* Default target of monsterspell is player */
    monster_target_y = player_ptr->y;
    monster_target_x = player_ptr->x;

    const auto max_range = AngbandSystem::get_instance().get_max_range();
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    for (const auto &pos : floor.get_area()) {
        const auto &grid = floor.get_grid(pos);
        if (!grid.is_mirror()) {
            continue;
        }

        const auto dist = Grid::calc_distance(p_pos, pos);
        const auto is_projectable = projectable(floor, p_pos, pos);
        if ((dist == 0) || (dist > max_range) || !grid.has_los() || !is_projectable) {
            continue;
        }

        mirror_y[mirror_num] = pos.y;
        mirror_x[mirror_num] = pos.x;
        mirror_num++;
    }

    if (mirror_num < 2) {
        return false;
    }

    point_x[0] = randint0(mirror_num);
    do {
        point_x[1] = randint0(mirror_num);
    } while (point_x[0] == point_x[1]);

    point_y[0] = mirror_y[point_x[0]];
    point_x[0] = mirror_x[point_x[0]];
    point_y[1] = mirror_y[point_x[1]];
    point_x[1] = mirror_x[point_x[1]];
    point_y[2] = player_ptr->y;
    point_x[2] = player_ptr->x;

    POSITION x = point_x[0] + point_x[1] + point_x[2];
    POSITION y = point_y[0] + point_y[1] + point_y[2];

    POSITION centersign = (point_x[0] * 3 - x) * (point_y[1] * 3 - y) - (point_y[0] * 3 - y) * (point_x[1] * 3 - x);
    if (centersign == 0) {
        return false;
    }

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
            const Pos2D pos(y, x);
            if ((centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) < 0)) {
                continue;
            }

            if ((centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) < 0)) {
                continue;
            }

            if ((centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) < 0)) {
                continue;
            }

            if (floor.has_los_at(pos) && projectable(floor, p_pos, pos)) {
                if (!(player_ptr->effects()->blindness().is_blind()) && panel_contains(y, x)) {
                    print_bolt_pict(player_ptr, y, x, y, x, AttributeType::MANA);
                    move_cursor_relative(y, x);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                }
            }
        }
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            const Pos2D pos(y, x);
            if (centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) < 0) {
                continue;
            }

            if (centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) < 0) {
                continue;
            }

            if (centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) < 0) {
                continue;
            }

            if (floor.has_los_at(pos) && projectable(floor, p_pos, pos)) {
                (void)affect_feature(player_ptr, 0, 0, y, x, dam, AttributeType::MANA);
            }
        }
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            const Pos2D pos(y, x);
            if (centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) < 0) {
                continue;
            }

            if (centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) < 0) {
                continue;
            }

            if (centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) < 0) {
                continue;
            }

            if (floor.has_los_at(pos) && projectable(floor, p_pos, pos)) {
                (void)affect_item(player_ptr, 0, 0, y, x, dam, AttributeType::MANA);
            }
        }
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            const Pos2D pos(y, x);
            if (centersign * ((point_x[0] - x) * (point_y[1] - y) - (point_y[0] - y) * (point_x[1] - x)) < 0) {
                continue;
            }

            if (centersign * ((point_x[1] - x) * (point_y[2] - y) - (point_y[1] - y) * (point_x[2] - x)) < 0) {
                continue;
            }

            if (centersign * ((point_x[2] - x) * (point_y[0] - y) - (point_y[2] - y) * (point_x[0] - x)) < 0) {
                continue;
            }

            if (floor.has_los_at(pos) && projectable(floor, p_pos, pos)) {
                constexpr auto flags = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP;
                (void)affect_monster(player_ptr, 0, 0, y, x, dam, AttributeType::MANA, flags, true);
            }
        }
    }

    if (one_in_(7)) {
        msg_print(_("鏡が結界に耐えきれず、壊れてしまった。", "The field broke a mirror"));
        SpellsMirrorMaster(player_ptr).remove_mirror(point_y[0], point_x[0]);
    }

    return true;
}

/*!
 * 幻惑の光 @ 鏡使いだけでなく混沌の戦士も使える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool confusing_light(PlayerType *player_ptr)
{
    msg_print(_("辺りを睨んだ...", "You glare at nearby monsters..."));
    slow_monsters(player_ptr, player_ptr->lev);
    stun_monsters(player_ptr, player_ptr->lev * 4);
    confuse_monsters(player_ptr, player_ptr->lev * 4);
    turn_monsters(player_ptr, player_ptr->lev * 4);
    stasis_monsters(player_ptr, player_ptr->lev * 4);
    return true;
}

/*
 * Set "multishadow", notice observable changes
 */
bool set_multishadow(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->multishadow && !do_dec) {
            if (player_ptr->multishadow > v) {
                return false;
            }
        } else if (!player_ptr->multishadow) {
            msg_print(_("あなたの周りに幻影が生まれた。", "Your Shadow enveloped you."));
            notice = true;
        }
    } else {
        if (player_ptr->multishadow) {
            msg_print(_("幻影が消えた。", "Your Shadow disappears."));
            notice = true;
        }
    }

    player_ptr->multishadow = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 一時的破片のオーラの継続時間をセットする / Set "dustrobe", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_dustrobe(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->dustrobe && !do_dec) {
            if (player_ptr->dustrobe > v) {
                return false;
            }
        } else if (!player_ptr->dustrobe) {
            msg_print(_("体が鏡のオーラで覆われた。", "You are enveloped by mirror shards."));
            notice = true;
        }
    } else {
        if (player_ptr->dustrobe) {
            msg_print(_("鏡のオーラが消えた。", "The mirror shards disappear."));
            notice = true;
        }
    }

    player_ptr->dustrobe = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 現在フロアに存在している鏡の数を数える / calculate mirrors
 * @return 鏡の枚数
 */
static int number_of_mirrors(const FloorType &floor)
{
    const auto has_mirror = [&](const Pos2D &pos) { return floor.get_grid(pos).is_mirror(); };
    return ranges::count_if(floor.get_area(), has_mirror);
}

/*!
 * @brief 鏡魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'Mirror magic'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_mirror_spell(PlayerType *player_ptr, MindMirrorMasterType spell)
{
    PLAYER_LEVEL plev = player_ptr->lev;
    int tmp;
    TIME_EFFECT t;
    const auto &grid = player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    switch (spell) {
    case MindMirrorMasterType::MIRROR_SEEING:
        tmp = grid.is_mirror() ? 4 : 0;
        if (plev + tmp > 4) {
            detect_monsters_normal(player_ptr, DETECT_RAD_DEFAULT);
        }
        if (plev + tmp > 18) {
            detect_monsters_invis(player_ptr, DETECT_RAD_DEFAULT);
        }
        if (plev + tmp > 28) {
            set_tim_esp(player_ptr, (TIME_EFFECT)plev, false);
        }
        if (plev + tmp > 38) {
            map_area(player_ptr, DETECT_RAD_MAP);
        }
        if (tmp == 0 && plev < 5) {
            msg_print(_("鏡がなくて集中できなかった！", "You need a mirror to concentrate!"));
        }
        break;
    case MindMirrorMasterType::MAKE_MIRROR:
        if (number_of_mirrors(*player_ptr->current_floor_ptr) < 4 + plev / 10) {
            const auto error = SpellsMirrorMaster(player_ptr).place_mirror();
            if (error) {
                msg_print(*error);
            }
        } else {
            msg_format(_("これ以上鏡は制御できない！", "There are too many mirrors to control!"));
        }

        break;
    case MindMirrorMasterType::DRIP_LIGHT: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        if (plev > 9 && grid.is_mirror()) {
            fire_beam(player_ptr, AttributeType::LITE, dir, Dice::roll(3 + ((plev - 1) / 5), 4));
        } else {
            fire_bolt(player_ptr, AttributeType::LITE, dir, Dice::roll(3 + ((plev - 1) / 5), 4));
        }

        break;
    }
    case MindMirrorMasterType::WRAPPED_MIRROR:
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case MindMirrorMasterType::MIRROR_LIGHT:
        (void)lite_area(player_ptr, Dice::roll(2, (plev / 2)), (plev / 10) + 1);
        break;
    case MindMirrorMasterType::WANDERING_MIRROR:
        teleport_player(player_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case MindMirrorMasterType::ROBE_DUST:
        set_dustrobe(player_ptr, 20 + randint1(20), false);
        break;
    case MindMirrorMasterType::BANISHING_MIRROR: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        (void)fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, plev);
        break;
    }
    case MindMirrorMasterType::MIRROR_CRASHING: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        fire_ball(player_ptr, AttributeType::SHARDS, dir, Dice::roll(8 + ((plev - 5) / 4), 8), (plev > 20 ? (plev - 20) / 8 + 1 : 0));
        break;
    }
    case MindMirrorMasterType::SLEEPING_MIRROR:
        for (const auto &pos : player_ptr->current_floor_ptr->get_area()) {
            if (player_ptr->current_floor_ptr->get_grid(pos).is_mirror()) {
                project(player_ptr, 0, 2, pos.y, pos.x, (int)plev, AttributeType::OLD_SLEEP,
                    (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI));
            }
        }

        break;
    case MindMirrorMasterType::SEEKER_RAY: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        SpellsMirrorMaster(player_ptr).seeker_ray(dir, Dice::roll(11 + (plev - 5) / 4, 8));
        break;
    }
    case MindMirrorMasterType::SEALING_MIRROR:
        SpellsMirrorMaster(player_ptr).seal_of_mirror(plev * 4 + 100);
        break;
    case MindMirrorMasterType::WATER_SHIELD:
        t = 20 + randint1(20);
        set_shield(player_ptr, t, false);
        if (plev > 31) {
            set_tim_reflect(player_ptr, t, false);
        }

        if (plev > 39) {
            set_resist_magic(player_ptr, t, false);
        }

        break;
    case MindMirrorMasterType::SUPER_RAY: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        SpellsMirrorMaster(player_ptr).super_ray(dir, 150 + randint1(2 * plev));
        break;
    }
    case MindMirrorMasterType::ILLUSION_LIGHT:
        tmp = grid.is_mirror() ? 4 : 3;
        slow_monsters(player_ptr, plev);
        stun_monsters(player_ptr, plev * tmp * 2);
        confuse_monsters(player_ptr, plev * tmp);
        turn_monsters(player_ptr, plev * tmp);
        stasis_monsters(player_ptr, plev * tmp);
        break;
    case MindMirrorMasterType::MIRROR_SHIFT:
        if (!grid.is_mirror()) {
            msg_print(_("鏡の国の場所がわからない！", "You cannot find out where the mirror is!"));
            break;
        }

        reserve_alter_reality(player_ptr, randint0(21) + 15);
        break;
    case MindMirrorMasterType::MIRROR_TUNNEL:
        msg_print(_("鏡の世界を通り抜け…  ", "You try to enter the mirror..."));
        return SpellsMirrorMaster(player_ptr).mirror_tunnel();
    case MindMirrorMasterType::RECALL_MIRROR:
        return recall_player(player_ptr, randint0(21) + 15);
    case MindMirrorMasterType::MULTI_SHADOW:
        set_multishadow(player_ptr, 6 + randint1(6), false);
        break;
    case MindMirrorMasterType::BINDING_FIELD:
        if (!binding_field(player_ptr, plev * 11 + 5)) {
            msg_print(_("適当な鏡を選べなかった！", "You were not able to choose suitable mirrors!"));
        }

        break;
    case MindMirrorMasterType::RUFFNOR_MIRROR:
        (void)set_invuln(player_ptr, randint1(4) + 4, false);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }

    return true;
}
