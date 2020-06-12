#include "view/display-lore-drops.h"
#include "monster-race/race-flags1.h"

void display_monster_drop_quantity(lore_type *lore_ptr)
{
    lore_ptr->drop_quantity = MAX(lore_ptr->drop_gold, lore_ptr->drop_item);
    if (lore_ptr->drop_quantity == 1) {
        hooked_roff(_("一つの", " a"));
#ifdef JP
#else
        lore_ptr->sin = TRUE;
#endif
    } else if (lore_ptr->drop_quantity == 2) {
        hooked_roff(_("一つか二つの", " one or two"));
    } else {
        hooked_roff(format(_(" %d 個までの", " up to %d"), lore_ptr->drop_quantity));
    }
}

void display_monster_drop_quality(lore_type* lore_ptr)
{
    if (lore_ptr->flags1 & RF1_DROP_GREAT) {
        lore_ptr->drop_quality = _("特別な", " exceptional");
    } else if (lore_ptr->flags1 & RF1_DROP_GOOD) {
        lore_ptr->drop_quality = _("上質な", " good");
#ifdef JP
#else
        lore_ptr->sin = FALSE;
#endif
    } else {
        lore_ptr->drop_quality = NULL;
    }
}
