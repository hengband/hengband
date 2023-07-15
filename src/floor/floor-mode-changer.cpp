#include "floor/floor-mode-changer.h"
#include "system/player-type-definition.h"

/*!
 * @brief フロア切り替え時の処理フラグを追加する / Prepare mode flags of changing floor
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mode 追加したい所持フラグ
 */
void prepare_change_floor_mode(PlayerType *player_ptr, BIT_FLAGS mode)
{
    player_ptr->change_floor_mode |= mode;
}
