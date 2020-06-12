#include "view/display-lore-magics.h"
#include "monster-race/race-flags2.h"
#include "term/term-color-types.h"

void display_monster_breath(lore_type *lore_ptr)
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

void display_monster_magic_types(lore_type *lore_ptr)
{
    lore_ptr->magic = FALSE;
    if (lore_ptr->vn == 0)
        return;

    lore_ptr->magic = TRUE;
    if (lore_ptr->breath) {
        hooked_roff(_("、なおかつ", ", and is also"));
    } else {
        hooked_roff(format(_("%^sは", "%^s is"), wd_he[lore_ptr->msex]));
    }

#ifdef JP
    if (lore_ptr->flags2 & (RF2_SMART))
        hook_c_roff(TERM_YELLOW, "的確に");

    hooked_roff("魔法を使うことができ、");
#else
    hooked_roff(" magical, casting spells");
    if (lore_ptr->flags2 & RF2_SMART)
        hook_c_roff(TERM_YELLOW, " intelligently");
#endif

    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != 0)
            hooked_roff("、");
#else
        if (n == 0)
            hooked_roff(" which ");
        else if (n < lore_ptr->vn - 1)
            hooked_roff(", ");
        else
            hooked_roff(" or ");
#endif
        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
    }

#ifdef JP
    hooked_roff("の呪文を唱えることがある");
#endif
}