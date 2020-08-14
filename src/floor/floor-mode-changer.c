#include "floor/floor-mode-changer.h"

/*!
 * @brief フロア切り替え時の処理フラグを追加する / Prepare mode flags of changing floor
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param mode 追加したい所持フラグ
 * @return なし
 */
void prepare_change_floor_mode(player_type *creature_ptr, BIT_FLAGS mode) { creature_ptr->change_floor_mode |= mode; }
