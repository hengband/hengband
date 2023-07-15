#include "spell-kind/spells-specific-bolt.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "spell-kind/spells-launcher.h"
#include "system/player-type-definition.h"

/*!
 * @brief 衰弱ボルト処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool hypodynamic_bolt(PlayerType *player_ptr, DIRECTION dir, int dam)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::HYPODYNAMIA, dir, dam, flg);
}

/*!
 * @brief 死の光線処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(効力はplev*200)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool death_ray(PlayerType *player_ptr, DIRECTION dir, PLAYER_LEVEL plev)
{
    BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
    return project_hook(player_ptr, AttributeType::DEATH_RAY, dir, plev * 200, flg);
}
