/*!
 * @brief 破邪魔法処理
 * @date 2020/06/05
 * @author Hourier
 */

#include "spell-realm/spells-crusade.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "spell-realm/spells-crusade.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <cmath>

/*!
 * @brief 破邪魔法「神の怒り」の処理としてターゲットを指定した後分解のボールを最大20回発生させる。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam ダメージ
 * @param rad 効力の半径
 * @return ターゲットを指定し、実行したならばTRUEを返す。
 */
bool cast_wrath_of_the_god(PlayerType *player_ptr, int dam, POSITION rad)
{
    int dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    const auto p_pos = player_ptr->get_position();
    auto pos_target = p_pos + Pos2DVec(99 * ddy[dir], 99 * ddx[dir]);
    if ((dir == 5) && target_okay(player_ptr)) {
        pos_target = { target_col, target_row };
    }

    auto pos = p_pos;
    auto &floor = *player_ptr->current_floor_ptr;
    while (true) {
        if (pos == pos_target) {
            break;
        }

        const auto pos_to = mmove2(pos, p_pos, pos_target);
        if (AngbandSystem::get_instance().get_max_range() <= Grid::calc_distance(p_pos, pos_to)) {
            break;
        }
        if (!floor.has_terrain_characteristics(pos_to, TerrainCharacteristics::PROJECT)) {
            break;
        }
        if ((dir != 5) && floor.get_grid(pos_to).has_monster()) {
            break;
        }

        pos = pos_to;
    }

    pos_target = pos;
    const auto b = 10 + randint1(10);
    for (auto i = 0; i < b; i++) {
        auto count = 20;
        auto pos_explode = pos_target;
        while (count--) {
            const auto y = pos_target.y - 5 + randint0(11);
            const auto x = pos_target.x - 5 + randint0(11);
            const auto dy = std::abs(pos_target.y - y);
            const auto dx = std::abs(pos_target.x - x);
            const auto d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
            if (d < 5) {
                pos_explode = { y, x };
                break;
            }
        }

        if (count < 0) {
            continue;
        }

        auto should_cast = in_bounds(&floor, pos_explode.y, pos_explode.x) && !cave_stop_disintegration(&floor, pos_explode.y, pos_explode.x);
        should_cast &= in_disintegration_range(&floor, pos_target.y, pos_target.x, pos_explode.y, pos_explode.x);
        if (!should_cast) {
            continue;
        }

        constexpr auto mode = PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
        project(player_ptr, 0, rad, pos_explode.y, pos_explode.x, dam, AttributeType::DISINTEGRATE, mode);
    }

    return true;
}

/*!
 * @brief 一時的聖なるのオーラの継続時間をセットする / Set "tim_sh_holy", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_holy(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_sh_holy && !do_dec) {
            if (player_ptr->tim_sh_holy > v) {
                return false;
            }
        } else if (!player_ptr->tim_sh_holy) {
            msg_print(_("体が聖なるオーラで覆われた。", "You are enveloped by a holy aura!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_sh_holy) {
            msg_print(_("聖なるオーラが消えた。", "The holy aura disappeared."));
            notice = true;
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    player_ptr->tim_sh_holy = v;
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
 * @brief 目には目をの残り時間をセットする / Set "tim_eyeeye", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す
 * @details 呪術領域でも使えるが、汎用性と行数の兼ね合いを考えて破邪側に入れた
 */
bool set_tim_eyeeye(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_eyeeye && !do_dec) {
            if (player_ptr->tim_eyeeye > v) {
                return false;
            }
        } else if (!player_ptr->tim_eyeeye) {
            msg_print(_("法の守り手になった気がした！", "You feel like a keeper of commandments!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_eyeeye) {
            msg_print(_("懲罰を執行することができなくなった。", "You lost your aura of retribution."));
            notice = true;
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    player_ptr->tim_eyeeye = v;
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
