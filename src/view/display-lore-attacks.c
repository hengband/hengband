#include "view/display-lore-attacks.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"

#ifdef JP
static void display_monster_blows_jp(lore_type *lore_ptr, int attack_numbers, int d1, int d2, int m)
{
    if (attack_numbers == 0) {
        hooked_roff(format("%^sは", wd_he[lore_ptr->msex]));
    }

    if (d1 && d2 && (lore_ptr->know_everything || know_damage(lore_ptr->r_idx, m))) {
        hooked_roff(format(" %dd%d ", d1, d2));
        hooked_roff("のダメージで");
    }

    if (!lore_ptr->p)
        lore_ptr->p = "何か奇妙なことをする";

    /* XXしてYYし/XXしてYYする/XXし/XXする */
    if (lore_ptr->q != NULL)
        jverb(lore_ptr->p, lore_ptr->jverb_buf, JVERB_TO);
    else if (attack_numbers != lore_ptr->count - 1)
        jverb(lore_ptr->p, lore_ptr->jverb_buf, JVERB_AND);
    else
        strcpy(lore_ptr->jverb_buf, lore_ptr->p);

    hooked_roff(lore_ptr->jverb_buf);
    if (lore_ptr->q) {
        if (attack_numbers != lore_ptr->count - 1)
            jverb(lore_ptr->q, lore_ptr->jverb_buf, JVERB_AND);
        else
            strcpy(lore_ptr->jverb_buf, lore_ptr->q);
        hooked_roff(lore_ptr->jverb_buf);
    }

    if (attack_numbers != lore_ptr->count - 1)
        hooked_roff("、");
}
#else

static void display_monster_blows_en(lore_type *lore_ptr, int attack_numbers, int d1, int d2, int m)
{
    if (attack_numbers == 0) {
        hooked_roff(format("%^s can ", wd_he[lore_ptr->msex]));
    } else if (attack_numbers < lore_ptr->count - 1) {
        hooked_roff(", ");
    } else {
        hooked_roff(", and ");
    }

    if (lore_ptr->p == NULL)
        lore_ptr->p = "do something weird";

    hooked_roff(lore_ptr->p);
    if (lore_ptr->q != NULL) {
        hooked_roff(" to ");
        hooked_roff(lore_ptr->q);
        if (d1 && d2 && (lore_ptr->know_everything || know_damage(lore_ptr->r_idx, m))) {
            hooked_roff(" with damage");
            hooked_roff(format(" %dd%d", d1, d2));
        }
    }
}
#endif

void display_monster_blows(lore_type *lore_ptr, int m, int attack_numbers)
{
    int d1 = lore_ptr->r_ptr->blow[m].d_dice;
    int d2 = lore_ptr->r_ptr->blow[m].d_side;
    void (*display_monster_blows_pf)(lore_type *, int, int, int, int) = _(display_monster_blows_jp, display_monster_blows_en);
    (*display_monster_blows_pf)(lore_ptr, attack_numbers, d1, d2, m);
}
