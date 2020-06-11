#include "lore/lore-util.h"

lore_type *initialize_lore_type(lore_type *lore_ptr, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
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
    return lore_ptr;
}
