#include "view/display-lore-magics.h"
#include "lore/lore-util.h"
#include "monster-race/race-flags2.h"
#include "system/monster-race-info.h"
#include "term/term-color-types.h"

void display_monster_breath(lore_type *lore_ptr)
{
    lore_ptr->breath = false;
    if (lore_ptr->vn <= 0) {
        return;
    }

    lore_ptr->breath = true;
    hooked_roff(format(_("%^sは", "%^s"), Who::who(lore_ptr->msex)));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != 0) {
            hooked_roff("や");
        }
#else
        if (n == 0) {
            hooked_roff(" may breathe ");
        } else if (n < lore_ptr->vn - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }
#endif
        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
    }

#ifdef JP
    hooked_roff("のブレスを吐くことがある");
#endif
}

void display_monster_magic_types(lore_type *lore_ptr)
{
    lore_ptr->magic = false;
    if (lore_ptr->vn == 0) {
        return;
    }

    lore_ptr->magic = true;
    if (lore_ptr->breath) {
        hooked_roff(_("、なおかつ", ", and is also"));
    } else {
        hooked_roff(format(_("%^sは", "%^s is"), Who::who(lore_ptr->msex)));
    }

#ifdef JP
    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::SMART)) {
        hook_c_roff(TERM_YELLOW, "的確に");
    }

    hooked_roff("魔法を使うことができ、");
#else
    hooked_roff(" magical, casting spells");
    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::SMART)) {
        hook_c_roff(TERM_YELLOW, " intelligently");
    }
#endif

    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != 0) {
            hooked_roff("、");
        }
#else
        if (n == 0) {
            hooked_roff(" which ");
        } else if (n < lore_ptr->vn - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }
#endif
        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
    }

#ifdef JP
    hooked_roff("の呪文を唱えることがある");
#endif
}

void display_mosnter_magic_possibility(lore_type *lore_ptr)
{
    if (!lore_ptr->breath && !lore_ptr->magic && !lore_ptr->shoot && !lore_ptr->rocket) {
        return;
    }

    int m = lore_ptr->r_ptr->r_cast_spell;
    int n = lore_ptr->r_ptr->freq_spell;
    if (m > 100 || lore_ptr->know_everything) {
        hooked_roff(format(_("(確率:1/%d)", "; 1 time in %d"), 100 / n));
    } else if (m) {
        n = ((n + 9) / 10) * 10;
        hooked_roff(format(_("(確率:約1/%d)", "; about 1 time in %d"), 100 / n));
    }

    hooked_roff(_("。", ".  "));
}
