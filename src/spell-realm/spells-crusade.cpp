/*!
 * @brief 破邪魔法処理
 * @date 2020/06/05
 * @author Hourier
 */

#include "spell-realm/spells-crusade.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/feature-flag-types.h"
#include "spell-realm/spells-crusade.h"
#include "spell/range-calc.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 破邪魔法「神の怒り」の処理としてターゲットを指定した後分解のボールを最大20回発生させる。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam ダメージ
 * @param rad 効力の半径
 * @return ターゲットを指定し、実行したならばTRUEを返す。
 */
bool cast_wrath_of_the_god(player_type *player_ptr, HIT_POINT dam, POSITION rad)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir))
        return false;

    POSITION tx = player_ptr->x + 99 * ddx[dir];
    POSITION ty = player_ptr->y + 99 * ddy[dir];
    if ((dir == 5) && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    POSITION x = player_ptr->x;
    POSITION y = player_ptr->y;
    POSITION nx, ny;
    while (true) {
        if ((y == ty) && (x == tx))
            break;

        ny = y;
        nx = x;
        mmove2(&ny, &nx, player_ptr->y, player_ptr->x, ty, tx);
        if (get_max_range(player_ptr) <= distance(player_ptr->y, player_ptr->x, ny, nx))
            break;
        if (!cave_has_flag_bold(player_ptr->current_floor_ptr, ny, nx, FF::PROJECT))
            break;
        if ((dir != 5) && player_ptr->current_floor_ptr->grid_array[ny][nx].m_idx != 0)
            break;

        x = nx;
        y = ny;
    }

    tx = x;
    ty = y;

    int b = 10 + randint1(10);
    for (int i = 0; i < b; i++) {
        int count = 20, d = 0;

        while (count--) {
            int dx, dy;

            x = tx - 5 + randint0(11);
            y = ty - 5 + randint0(11);

            dx = (tx > x) ? (tx - x) : (x - tx);
            dy = (ty > y) ? (ty - y) : (y - ty);

            d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
            if (d < 5)
                break;
        }

        if (count < 0)
            continue;

        if (!in_bounds(player_ptr->current_floor_ptr, y, x) || cave_stop_disintegration(player_ptr->current_floor_ptr, y, x)
            || !in_disintegration_range(player_ptr->current_floor_ptr, ty, tx, y, x))
            continue;

        project(player_ptr, 0, rad, y, x, dam, GF_DISINTEGRATE, PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL);
    }

    return true;
}

/*!
 * @brief 一時的聖なるのオーラの継続時間をセットする / Set "tim_sh_holy", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_holy(player_type *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->tim_sh_holy && !do_dec) {
            if (player_ptr->tim_sh_holy > v)
                return false;
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

    player_ptr->tim_sh_holy = v;
    player_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
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
bool set_tim_eyeeye(player_type *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (player_ptr->is_dead)
        return false;

    if (v) {
        if (player_ptr->tim_eyeeye && !do_dec) {
            if (player_ptr->tim_eyeeye > v)
                return false;
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

    player_ptr->tim_eyeeye = v;
    player_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return false;

    if (disturb_state)
        disturb(player_ptr, false, false);
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
    return true;
}
