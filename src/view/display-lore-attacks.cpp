#include "view/display-lore-attacks.h"
#include "locale/japanese.h"
#include "lore/combat-types-setter.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-attack/monster-attack-types.h"
#include "monster-race/race-flags1.h"
#include "system/monster-race-definition.h"
#include "term/term-color-types.h"

#ifdef JP
/*!
 * @brief [日本語]モンスター打撃の1回分を出力する
 * @param lore_ptr 思い出情報へのポインタ
 * @param attack_numbers 打撃の最大回数
 * @param d1 ダメージダイス数
 * @param d2 ダメージダイス面
 * @param m 打撃の何番目か
 */
static void display_monster_blow_jp(lore_type *lore_ptr, int attack_numbers, int d1, int d2, int m)
{
    if (attack_numbers == 0) {
        hooked_roff(format("%^sは", Who::who(lore_ptr->msex)));
    }

    if (d1 && d2 && (lore_ptr->know_everything || know_damage(lore_ptr->r_idx, m))) {
        hook_c_roff(TERM_L_WHITE, format(" %dd%d ", d1, d2));
        hooked_roff("のダメージで");
    }

    if (!lore_ptr->p) {
        lore_ptr->p = "何か奇妙なことをする";
        lore_ptr->pc = TERM_VIOLET;
    }

    /* XXしてYYし/XXしてYYする/XXし/XXする */
    if (lore_ptr->q != nullptr)
        jverb(lore_ptr->p, lore_ptr->jverb_buf, JVERB_TO);
    else if (attack_numbers != lore_ptr->count - 1)
        jverb(lore_ptr->p, lore_ptr->jverb_buf, JVERB_AND);
    else
        strcpy(lore_ptr->jverb_buf, lore_ptr->p);

    hook_c_roff(lore_ptr->pc, lore_ptr->jverb_buf);

    if (lore_ptr->q) {
        if (attack_numbers != lore_ptr->count - 1)
            jverb(lore_ptr->q, lore_ptr->jverb_buf, JVERB_AND);
        else
            strcpy(lore_ptr->jverb_buf, lore_ptr->q);

        hook_c_roff(lore_ptr->qc, lore_ptr->jverb_buf);
    }

    if (attack_numbers != lore_ptr->count - 1)
        hooked_roff("、");
}
#else
/*!
 * @brief [英語]モンスター打撃の1回分を出力する
 * @param lore_ptr 思い出情報へのポインタ
 * @param attack_numbers 打撃の最大回数
 * @param d1 ダメージダイス数
 * @param d2 ダメージダイス面
 * @param m 打撃の何番目か
 */
static void display_monster_blow_en(lore_type *lore_ptr, int attack_numbers, int d1, int d2, int m)
{
    if (attack_numbers == 0) {
        hooked_roff(format("%^s can ", Who::who(lore_ptr->msex)));
    } else if (attack_numbers < lore_ptr->count - 1) {
        hooked_roff(", ");
    } else {
        hooked_roff(", and ");
    }

    if (lore_ptr->p == nullptr) {
        lore_ptr->p = "do something weird";
        lore_ptr->pc = TERM_VIOLET;
    }

    hook_c_roff(lore_ptr->pc, lore_ptr->p);
    if (lore_ptr->q != nullptr) {
        hooked_roff(" to ");
        hook_c_roff(lore_ptr->qc, lore_ptr->q);
        if (d1 && d2 && (lore_ptr->know_everything || know_damage(lore_ptr->r_idx, m))) {
            hooked_roff(" with damage");
            hook_c_roff(TERM_L_WHITE, format(" %dd%d", d1, d2));
        }
    }
}
#endif

/*!
 * @brief モンスター打撃の1回分を出力する(日英切替への踏み台)
 * @param lore_ptr 思い出情報へのポインタ
 * @param m 打撃の何番目か
 * @param attack_numbers 打撃の最大回数
 */
void display_monster_blow(lore_type *lore_ptr, int m, int attack_numbers)
{
    int d1 = lore_ptr->r_ptr->blow[m].d_dice;
    int d2 = lore_ptr->r_ptr->blow[m].d_side;
    void (*display_monster_blows_pf)(lore_type *, int, int, int, int) = _(display_monster_blow_jp, display_monster_blow_en);
    (*display_monster_blows_pf)(lore_ptr, attack_numbers, d1, d2, m);
}

/*!
 * @brief モンスターの思い出に打撃に関する情報を出力する
 * @param lore_ptr 思い出情報へのポインタ
 */
void display_monster_blows(lore_type *lore_ptr)
{
    const int max_attack_numbers = 4;
    for (int m = 0; m < max_attack_numbers; m++) {
        if (!lore_ptr->r_ptr->blow[m].method || (lore_ptr->r_ptr->blow[m].method == RBM_SHOOT))
            continue;

        if (lore_ptr->r_ptr->r_blows[m] || lore_ptr->know_everything)
            lore_ptr->count++;
    }

    int attack_numbers = 0;
    for (int m = 0; m < max_attack_numbers; m++) {
        if (!lore_ptr->r_ptr->blow[m].method || (lore_ptr->r_ptr->blow[m].method == RBM_SHOOT) || (((lore_ptr->r_ptr->r_blows[m] == 0) && !lore_ptr->know_everything)))
            continue;

        set_monster_blow_method(lore_ptr, m);
        set_monster_blow_effect(lore_ptr, m);
        display_monster_blow(lore_ptr, m, attack_numbers);
        attack_numbers++;
    }

    if (attack_numbers > 0) {
        hooked_roff(_("。", ".  "));
    } else if (lore_ptr->flags1 & RF1_NEVER_BLOW) {
        hooked_roff(format(_("%^sは物理的な攻撃方法を持たない。", "%^s has no physical attacks.  "), Who::who(lore_ptr->msex)));
    } else {
        hooked_roff(format(_("%s攻撃については何も知らない。", "Nothing is known about %s attack.  "), Who::whose(lore_ptr->msex)));
    }
}
