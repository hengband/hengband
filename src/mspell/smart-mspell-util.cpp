#include "mspell/smart-mspell-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"

msr_type *initialize_msr_type(player_type *target_ptr, msr_type *msr_ptr, MONSTER_IDX m_idx, const u32b f4p, const u32b f5p, const u32b f6p)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    msr_ptr->r_ptr = &r_info[m_ptr->r_idx];
    msr_ptr->f4 = f4p;
    msr_ptr->f5 = f5p;
    msr_ptr->f6 = f6p;
    msr_ptr->smart = 0L;
    return msr_ptr;
}

/*!
 * @brief モンスターがプレイヤーの弱点をついた選択を取るかどうかの判定 /
 * Internal probability routine
 * @param r_ptr モンスター種族の構造体参照ポインタ
 * @param prob 基本確率(%)
 * @return 適した選択を取るならばTRUEを返す。
 */
bool int_outof(monster_race *r_ptr, PERCENTAGE prob)
{
    if (!(r_ptr->flags2 & RF2_SMART))
        prob = prob / 2;

    return (randint0(100) < prob);
}
