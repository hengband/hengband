#include "view/display-lore-drops.h"

void display_monster_drop_numbers(lore_type *lore_ptr)
{
    int n = MAX(lore_ptr->drop_gold, lore_ptr->drop_item);
    if (n == 1) {
        hooked_roff(_("一つの", " a"));
#ifdef JP
#else
        lore_ptr->sin = TRUE;
#endif
    } else if (n == 2) {
        hooked_roff(_("一つか二つの", " one or two"));
    } else {
        hooked_roff(format(_(" %d 個までの", " up to %d"), n));
    }
}
