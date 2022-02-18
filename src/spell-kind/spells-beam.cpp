#include "spell-kind/spells-beam.h"
#include "effect/effect-characteristics.h"
#include "effect/attribute-types.h"
#include "spell-kind/spells-launcher.h"
#include "system/player-type-definition.h"

/*!
 * @brief 岩石溶解処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_to_mud(PlayerType *player_ptr, DIRECTION dir, int dam)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    return project_hook(player_ptr, AttributeType::KILL_WALL, dir, dam, flg);
}

/*!
 * @brief 魔法の施錠処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wizard_lock(PlayerType *player_ptr, DIRECTION dir)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    return project_hook(player_ptr, AttributeType::JAM_DOOR, dir, 20 + randint1(30), flg);
}

/*!
 * @brief ドア破壊処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_door(PlayerType *player_ptr, DIRECTION dir)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
    return project_hook(player_ptr, AttributeType::KILL_DOOR, dir, 0, flg);
}

/*!
 * @brief トラップ解除処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_trap(PlayerType *player_ptr, DIRECTION dir)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
    return project_hook(player_ptr, AttributeType::KILL_TRAP, dir, 0, flg);
}
