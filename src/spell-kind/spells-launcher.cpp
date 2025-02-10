#include "spell-kind/spells-launcher.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/geometry.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "util/bit-flags-calculator.h"
#include "util/dice.h"

/*!
 * @brief ボール系スペルの発動 / Cast a ball spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param rad 半径
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 * </pre>
 */
bool fire_ball(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad, std::optional<CapturedMonsterType *> cap_mon_ptr)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    if (typ == AttributeType::CHARM_LIVING) {
        flg |= PROJECT_HIDE;
    }

    auto [ty, tx] = player_ptr->get_position() + Direction(dir).vec() * 99;

    if ((dir == 5) && target_okay(player_ptr)) {
        flg &= ~(PROJECT_STOP);
        tx = target_col;
        ty = target_row;
    }

    return project(player_ptr, 0, rad, ty, tx, dam, typ, flg, cap_mon_ptr).notice;
}

/*!
 * @brief ブレス系スペルの発動 / Cast a breath spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param rad 半径
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 * </pre>
 */
bool fire_breath(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_BREATH;

    auto [ty, tx] = player_ptr->get_position() + Direction(dir).vec() * 99;

    if ((dir == 5) && target_okay(player_ptr)) {
        reset_bits(flg, PROJECT_STOP);
        tx = target_col;
        ty = target_row;
    }

    return project(player_ptr, 0, rad, ty, tx, dam, typ, flg).notice;
}

/*!
 * @brief ロケット系スペルの発動(詳細な差は確認中) / Cast a ball spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param rad 半径
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 * </pre>
 */
bool fire_rocket(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad)
{
    auto [ty, tx] = player_ptr->get_position() + Direction(dir).vec() * 99;
    if ((dir == 5) && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    return project(player_ptr, 0, rad, ty, tx, dam, typ, flg).notice;
}

/*!
 * @brief ボール(ハイド)系スペルの発動 / Cast a ball spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param rad 半径
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 * </pre>
 */
bool fire_ball_hide(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad)
{
    auto [ty, tx] = player_ptr->get_position() + Direction(dir).vec() * 99;
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_HIDE;
    if ((dir == 5) && target_okay(player_ptr)) {
        flg &= ~(PROJECT_STOP);
        tx = target_col;
        ty = target_row;
    }

    return project(player_ptr, 0, rad, ty, tx, dam, typ, flg).notice;
}

/*!
 * @brief メテオ系スペルの発動 / Cast a meteor spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param src_idx スぺル詠唱者のモンスターID(0=プレイヤー)
 * @param typ 効果属性
 * @param dam 威力
 * @param rad 半径
 * @param y 中心点Y座標
 * @param x 中心点X座標
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Cast a meteor spell, defined as a ball spell cast by an arbitary monster,
 * player, or outside source, that starts out at an arbitrary location, and
 * leaving no trail from the "caster" to the target.  This function is
 * especially useful for bombardments and similar. -LM-
 * Option to hurt the player.
 * </pre>
 */
bool fire_meteor(PlayerType *player_ptr, MONSTER_IDX src_idx, AttributeType typ, POSITION y, POSITION x, int dam, POSITION rad)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    return project(player_ptr, src_idx, rad, y, x, dam, typ, flg).notice;
}

/*!
 * @brief ブラスト系スペルの発動 / Cast a blast spell
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dice 威力ダイス
 * @param num 基本回数
 * @param dev 回数分散
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fire_blast(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, const Dice &dice, int num, int dev)
{
    POSITION ty, tx, y, x;
    POSITION ly, lx;
    if (dir == 5) {
        tx = target_col;
        ty = target_row;

        lx = 20 * (tx - player_ptr->x) + player_ptr->x;
        ly = 20 * (ty - player_ptr->y) + player_ptr->y;
    } else {
        const auto vec = Direction(dir).vec();
        ly = ty = player_ptr->y + 20 * vec.y;
        lx = tx = player_ptr->x + 20 * vec.x;
    }

    const auto ld = Grid::calc_distance(player_ptr->get_position(), { ly, lx });
    BIT_FLAGS flg = PROJECT_FAST | PROJECT_THRU | PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE | PROJECT_GRID;
    bool result = true;
    for (int i = 0; i < num; i++) {
        while (true) {
            /* Get targets for some bolts */
            y = rand_spread(ly, ld * dev / 20);
            x = rand_spread(lx, ld * dev / 20);

            if (Grid::calc_distance({ ly, lx }, { y, x }) <= ld * dev / 20) {
                break;
            }
        }

        /* Analyze the "dir" and the "target". */
        const auto proj_res = project(player_ptr, 0, 0, y, x, dice.roll(), typ, flg);
        if (!proj_res.notice) {
            result = false;
        }
    }

    return result;
}

/*!
 * @brief ボルト系スペルの発動 / Cast a bolt spell.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, as a "bolt".
 * Affect monsters and grids (not objects).
 * </pre>
 */
bool fire_bolt(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_GRID;
    if (typ != AttributeType::MONSTER_SHOOT) {
        flg |= PROJECT_REFLECTABLE;
    }
    return project_hook(player_ptr, typ, dir, dam, flg);
}

/// @todo Direction型へ移行中のラッパー。最終的に完全に置き換える予定。
bool fire_bolt(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam)
{
    return fire_bolt(player_ptr, typ, dir.dir(), dam);
}

/*!
 * @brief ビーム系スペルの発動 / Cast a beam spell.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Pass through monsters, as a "beam".
 * Affect monsters, grids and objects.
 * </pre>
 */
bool fire_beam(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM;
    return project_hook(player_ptr, typ, dir, dam, flg);
}

/*!
 * @brief 確率に応じたボルト系/ビーム系スペルの発動 / Cast a bolt spell, or rarely, a beam spell.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param prob ビーム化する確率(%)
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Pass through monsters, as a "beam".
 * Affect monsters, grids and objects.
 * </pre>
 */
bool fire_bolt_or_beam(PlayerType *player_ptr, PERCENTAGE prob, AttributeType typ, DIRECTION dir, int dam)
{
    if (evaluate_percent(prob)) {
        return (fire_beam(player_ptr, typ, dir, dam));
    }

    return (fire_bolt(player_ptr, typ, dir, dam));
}

/*!
 * @brief 指定方向に飛び道具を飛ばす (フラグ任意指定) / Apply a "project()" in a direction (or at the target)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param flg フラグ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool project_hook(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, BIT_FLAGS flg)
{
    flg |= (PROJECT_THRU);
    const auto pos = ((dir == 5) && target_okay(player_ptr)) ? Pos2D(target_row, target_col) : player_ptr->get_neighbor(dir);
    return project(player_ptr, 0, 0, pos.y, pos.x, dam, typ, flg).notice;
}
