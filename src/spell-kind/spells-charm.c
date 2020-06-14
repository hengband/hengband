#include "spell-kind/spells-charm.h"
#include "effect/effect-characteristics.h"
#include "spell-kind/spells-launcher.h"
#include "spell/spell-types.h"

/*!
 * @brief チャーム・モンスター(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monster(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return (project_hook(caster_ptr, GF_CHARM, dir, plev, flg));
}

/*!
 * @brief アンデッド支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_undead(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return (project_hook(caster_ptr, GF_CONTROL_UNDEAD, dir, plev, flg));
}

/*!
 * @brief 悪魔支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_demon(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return (project_hook(caster_ptr, GF_CONTROL_DEMON, dir, plev, flg));
}

/*!
 * @brief 動物支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animal(player_type *caster_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
    return (project_hook(caster_ptr, GF_CONTROL_ANIMAL, dir, plev, flg));
}
