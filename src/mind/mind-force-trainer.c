#include "mind/mind-force-trainer.h"

/*!
 * @brief 練気術師が「練気」で溜めた気の量を返す
 * @param caster_ptr プレーヤーの参照ポインタ
 * @return 現在溜まっている気の量
 */
MAGIC_NUM1 get_current_ki(player_type *caster_ptr)
{
    return caster_ptr->magic_num1[0];
}

/*!
 * @brief 練気術師において、気を溜める
 * @param caster_ptr プレーヤーの参照ポインタ
 * @param is_reset TRUEなら気の量をkiにセットし、FALSEなら加減算を行う
 * @param ki 気の量
 * @return なし
 */
void set_current_ki(player_type *caster_ptr, bool is_reset, MAGIC_NUM1 ki)
{
    if (is_reset) {
        caster_ptr->magic_num1[0] = ki;
        return;
    }

    caster_ptr->magic_num1[0] += ki;
}
