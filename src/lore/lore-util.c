#include "lore/lore-util.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

concptr wd_he[3] = { _("それ", "it"), _("彼", "he"), _("彼女", "she") };
concptr wd_his[3] = { _("それの", "its"), _("彼の", "his"), _("彼女の", "her") };

/*
 * Prepare hook for c_roff(). It will be changed for spoiler generation in wizard1.c.
 */
hook_c_roff_pf hook_c_roff = c_roff;

lore_type *initialize_lore_type(lore_type *lore_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
#ifdef JP
#else
    lore_ptr->sin = FALSE;
#endif
    lore_ptr->r_idx = r_idx;
    lore_ptr->nightmare = ironman_nightmare && !(mode & 0x02);
    lore_ptr->r_ptr = &r_info[r_idx];
    lore_ptr->speed = lore_ptr->nightmare ? lore_ptr->r_ptr->speed + 5 : lore_ptr->r_ptr->speed;
    lore_ptr->drop_gold = lore_ptr->r_ptr->r_drop_gold;
    lore_ptr->drop_item = lore_ptr->r_ptr->r_drop_item;
    lore_ptr->flags1 = (lore_ptr->r_ptr->flags1 & lore_ptr->r_ptr->r_flags1);
    lore_ptr->flags2 = (lore_ptr->r_ptr->flags2 & lore_ptr->r_ptr->r_flags2);
    lore_ptr->flags3 = (lore_ptr->r_ptr->flags3 & lore_ptr->r_ptr->r_flags3);
    lore_ptr->flags4 = (lore_ptr->r_ptr->flags4 & lore_ptr->r_ptr->r_flags4);
    lore_ptr->a_ability_flags1 = (lore_ptr->r_ptr->a_ability_flags1 & lore_ptr->r_ptr->r_flags5);
    lore_ptr->a_ability_flags2 = (lore_ptr->r_ptr->a_ability_flags2 & lore_ptr->r_ptr->r_flags6);
    lore_ptr->flags7 = (lore_ptr->r_ptr->flags7 & lore_ptr->r_ptr->flags7);
    lore_ptr->flagsr = (lore_ptr->r_ptr->flagsr & lore_ptr->r_ptr->r_flagsr);
    lore_ptr->reinforce = FALSE;
    lore_ptr->know_everything = FALSE;
    lore_ptr->mode = mode;
    lore_ptr->old = FALSE;
    lore_ptr->count = 0;
    return lore_ptr;
}

/*!
 * @brief モンスターの思い出メッセージをあらかじめ指定された関数ポインタに基づき出力する
 * @param str 出力文字列
 * @return なし
 */
void hooked_roff(concptr str) { hook_c_roff(TERM_WHITE, str); }
