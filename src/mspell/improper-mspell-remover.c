#include "mspell/improper-mspell-remover.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster/smart-learn-types.h"
#include "mspell/element-resistance-checker.h"
#include "mspell/high-resistance-checker.h"
#include "mspell/smart-mspell-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"

static void add_cheat_remove_flags(player_type *target_ptr, msr_type *msr_ptr)
{
    if (!smart_cheat)
        return;

    add_cheat_remove_flags_element(target_ptr, msr_ptr);
    add_cheat_remove_flags_others(target_ptr, msr_ptr);
}

/*!
 * @brief モンスターの魔法一覧から戦術的に適さない魔法を除外する /
 * Remove the "bad" spells from a spell list
 * @param m_idx モンスターの構造体参照ポインタ
 * @param f4p モンスター魔法のフラグリスト1
 * @param f5p モンスター魔法のフラグリスト2
 * @param f6p モンスター魔法のフラグリスト3
 * @return なし
 */
void remove_bad_spells(MONSTER_IDX m_idx, player_type *target_ptr, u32b *f4p, u32b *f5p, u32b *f6p)
{
    msr_type tmp_msr;
    msr_type *msr_ptr = initialize_msr_type(target_ptr, &tmp_msr, m_idx, *f4p, *f5p, *f6p);
    if (msr_ptr->r_ptr->flags2 & RF2_STUPID)
        return;

    if (!smart_cheat && !smart_learn)
        return;

    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    if (smart_learn) {
        if (m_ptr->smart && (randint0(100) < 1))
            m_ptr->smart &= SM_FRIENDLY | SM_PET | SM_CLONED;

        msr_ptr->smart = m_ptr->smart;
    }

    add_cheat_remove_flags(target_ptr, msr_ptr);
    if (!msr_ptr->smart)
        return;

    check_element_resistance(msr_ptr);
    check_high_resistances(target_ptr, msr_ptr);
    *f4p = msr_ptr->f4;
    *f5p = msr_ptr->f5;
    *f6p = msr_ptr->f6;
}
