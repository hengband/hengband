﻿/*!
 * @brief 突然変異の各種計算 / Calculations for mutation
 * @date 2014/01/11
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply
 * 2014 Deskull rearranged comment for Doxygen
 * 2020 Hourier rearranged for dividing processes.
 */

#include "mutation/mutation-calculator.h"
#include "mutation/mutation-flag-types.h"

 /*!
 * @brief 現在プレイヤー得ている突然変異の数を返す。
 * @return 現在得ている突然変異の数
 */
static int count_mutations(player_type *creature_ptr)
{
    return count_bits(creature_ptr->muta1) + count_bits(creature_ptr->muta2) + count_bits(creature_ptr->muta3);
}

/*!
 * @brief 突然変異による自然回復ペナルティをパーセント値で返す /
 * Return the modifier to the regeneration rate (in percent)
 * @return ペナルティ修正(%)
 * @details
 * Beastman get 10 "free" mutations and only 5% decrease per additional mutation.
 * Max 90% decrease in regeneration speed.
 */
int calc_mutant_regenerate_mod(player_type *creature_ptr)
{
    int regen;
    int mod = 10;
    int count = count_mutations(creature_ptr);
    if (creature_ptr->pseikaku == PERSONALITY_LUCKY)
        count--;

    if (creature_ptr->prace == RACE_BEASTMAN) {
        count -= 10;
        mod = 5;
    }

    if (count <= 0)
        return 100;

    regen = 100 - count * mod;
    if (regen < 10)
        regen = 10;

    return (regen);
}
