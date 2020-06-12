#include "view/display-lore-magics.h"

void diaplay_monster_breath(lore_type *lore_ptr)
{
    lore_ptr->breath = FALSE;
    if (lore_ptr->vn <= 0)
        return;

    lore_ptr->breath = TRUE;
    hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != 0)
            hooked_roff("や");
#else
        if (n == 0)
            hooked_roff(" may breathe ");
        else if (n < lore_ptr->vn - 1)
            hooked_roff(", ");
        else
            hooked_roff(" or ");
#endif
        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
    }

#ifdef JP
    hooked_roff("のブレスを吐くことがある");
#endif
}
