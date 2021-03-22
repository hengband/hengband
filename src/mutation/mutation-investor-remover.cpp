#include "mutation/mutation-investor-remover.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "mutation/gain-mutation-switcher.h"
#include "mutation/lose-mutation-switcher.h"
#include "mutation/mutation-calculator.h" //!< @todo calc_mutant_regenerate_mod() が相互依存している、後で消す.
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-util.h"
#include "player-info/avatar.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static void sweep_gain_mutation(player_type *creature_ptr, glm_type *gm_ptr)
{
    int attempts_left = 20;
    if (gm_ptr->choose_mut)
        attempts_left = 1;

    while (attempts_left--) {
        switch_gain_mutation(creature_ptr, gm_ptr);
        if ((gm_ptr->muta_class != NULL) && (gm_ptr->muta_which != 0) && none_bits(*gm_ptr->muta_class, gm_ptr->muta_which))
            gm_ptr->muta_chosen = TRUE;

        if (gm_ptr->muta_chosen)
            break;
    }
}

static void race_dependent_mutation(player_type *creature_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->choose_mut != 0)
        return;

    if (creature_ptr->prace == RACE_VAMPIRE && none_bits(creature_ptr->muta1, MUT1_HYPN_GAZE) && (randint1(10) < 7)) {
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_HYPN_GAZE;
        gm_ptr->muta_desc = _("眼が幻惑的になった...", "Your eyes look mesmerizing...");
        return;
    }

    if (creature_ptr->prace == RACE_IMP && none_bits(creature_ptr->muta2, MUT2_HORNS) && (randint1(10) < 7)) {
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_HORNS;
        gm_ptr->muta_desc = _("角が額から生えてきた！", "Horns pop forth into your forehead!");
        return;
    }

    if (creature_ptr->prace == RACE_YEEK && none_bits(creature_ptr->muta1, MUT1_SHRIEK) && (randint1(10) < 7)) {
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_SHRIEK;
        gm_ptr->muta_desc = _("声質がかなり強くなった。", "Your vocal cords get much tougher.");
        return;
    }

    if (creature_ptr->prace == RACE_BEASTMAN && none_bits(creature_ptr->muta1, MUT1_POLYMORPH) && (randint1(10) < 2)) {
        gm_ptr->muta_class = &(creature_ptr->muta1);
        gm_ptr->muta_which = MUT1_POLYMORPH;
        gm_ptr->muta_desc = _("あなたの肉体は変化できるようになった、", "Your body seems mutable.");
        return;
    }

    if (creature_ptr->prace == RACE_MIND_FLAYER && none_bits(creature_ptr->muta2, MUT2_TENTACLES) && (randint1(10) < 7)) {
        gm_ptr->muta_class = &(creature_ptr->muta2);
        gm_ptr->muta_which = MUT2_TENTACLES;
        gm_ptr->muta_desc = _("邪悪な触手が口の周りに生えた。", "Evil-looking tentacles sprout from your mouth.");
    }
}

static void neutralize_base_status(player_type *creature_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == MUT3_PUNY) {
        if (any_bits(creature_ptr->muta3, MUT3_HYPER_STR)) {
            msg_print(_("あなたはもう超人的に強くはない！", "You no longer feel super-strong!"));
            reset_bits(creature_ptr->muta3, MUT3_HYPER_STR);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_HYPER_STR) {
        if (any_bits(creature_ptr->muta3, MUT3_PUNY)) {
            msg_print(_("あなたはもう虚弱ではない！", "You no longer feel puny!"));
            reset_bits(creature_ptr->muta3, MUT3_PUNY);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_MORONIC) {
        if (any_bits(creature_ptr->muta3, MUT3_HYPER_INT)) {
            msg_print(_("あなたの脳はもう生体コンピュータではない。", "Your brain is no longer a living computer."));
            reset_bits(creature_ptr->muta3, MUT3_HYPER_INT);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_HYPER_INT) {
        if (any_bits(creature_ptr->muta3, MUT3_MORONIC)) {
            msg_print(_("あなたはもう精神薄弱ではない。", "You are no longer moronic."));
            reset_bits(creature_ptr->muta3, MUT3_MORONIC);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_IRON_SKIN) {
        if (any_bits(creature_ptr->muta3, MUT3_SCALES)) {
            msg_print(_("鱗がなくなった。", "You lose your scales."));
            reset_bits(creature_ptr->muta3, MUT3_SCALES);
        }

        if (any_bits(creature_ptr->muta3, MUT3_FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh rots no longer."));
            reset_bits(creature_ptr->muta3, MUT3_FLESH_ROT);
        }

        if (any_bits(creature_ptr->muta3, MUT3_WART_SKIN)) {
            msg_print(_("肌のイボイボがなくなった。", "You lose your warts."));
            reset_bits(creature_ptr->muta3, MUT3_WART_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_WART_SKIN || gm_ptr->muta_which == MUT3_SCALES || gm_ptr->muta_which == MUT3_FLESH_ROT) {
        if (any_bits(creature_ptr->muta3, MUT3_IRON_SKIN)) {
            msg_print(_("あなたの肌はもう鉄ではない。", "Your skin is no longer made of steel."));
            reset_bits(creature_ptr->muta3, MUT3_IRON_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_FEARLESS) {
        if (any_bits(creature_ptr->muta2, MUT2_COWARDICE)) {
            msg_print(_("臆病でなくなった。", "You are no longer cowardly."));
            reset_bits(creature_ptr->muta2, MUT2_COWARDICE);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_FLESH_ROT) {
        if (any_bits(creature_ptr->muta3, MUT3_REGEN)) {
            msg_print(_("急速に回復しなくなった。", "You stop regenerating."));
            reset_bits(creature_ptr->muta3, MUT3_REGEN);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_REGEN) {
        if (any_bits(creature_ptr->muta3, MUT3_FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh stops rotting."));
            reset_bits(creature_ptr->muta3, MUT3_FLESH_ROT);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_LIMBER) {
        if (any_bits(creature_ptr->muta3, MUT3_ARTHRITIS)) {
            msg_print(_("関節が痛くなくなった。", "Your joints stop hurting."));
            reset_bits(creature_ptr->muta3, MUT3_ARTHRITIS);
        }

        return;
    }

    if (gm_ptr->muta_which == MUT3_ARTHRITIS) {
        if (any_bits(creature_ptr->muta3, MUT3_LIMBER)) {
            msg_print(_("あなたはしなやかでなくなった。", "You no longer feel limber."));
            reset_bits(creature_ptr->muta3, MUT3_LIMBER);
        }

        return;
    }
}

static void neutralize_other_status(player_type *creature_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == MUT2_COWARDICE) {
        if (any_bits(creature_ptr->muta3, MUT3_FEARLESS)) {
            msg_print(_("恐れ知らずでなくなった。", "You no longer feel fearless."));
            reset_bits(creature_ptr->muta3, (MUT3_FEARLESS));
        }
    }

    if (gm_ptr->muta_which == MUT2_BEAK) {
        if (any_bits(creature_ptr->muta2, MUT2_TRUNK)) {
            msg_print(_("あなたの鼻はもう象の鼻のようではなくなった。", "Your nose is no longer elephantine."));
            reset_bits(creature_ptr->muta2, (MUT2_TRUNK));
        }
    }

    if (gm_ptr->muta_which == MUT2_TRUNK) {
        if (any_bits(creature_ptr->muta2, MUT2_BEAK)) {
            msg_print(_("硬いクチバシがなくなった。", "You no longer have a hard beak."));
            reset_bits(creature_ptr->muta2, (MUT2_BEAK));
        }
    }
}

/*!
 * @brief プレイヤーに突然変異を与える
 * @param choose_mut 与えたい突然変異のID、0ならばランダムに選択
 * @return なし
 */
bool gain_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut)
{
    glm_type tmp_gm;
    glm_type *gm_ptr = initialize_glm_type(&tmp_gm, choose_mut);
    sweep_gain_mutation(creature_ptr, gm_ptr);
    if (!gm_ptr->muta_chosen) {
        msg_print(_("普通になった気がする。", "You feel normal."));
        return FALSE;
    }

    chg_virtue(creature_ptr, V_CHANCE, 1);
    race_dependent_mutation(creature_ptr, gm_ptr);
    msg_print(_("突然変異した！", "You mutate!"));
    msg_print(gm_ptr->muta_desc);
    if (gm_ptr->muta_class != NULL)
        set_bits(*gm_ptr->muta_class, gm_ptr->muta_which);

    if (gm_ptr->muta_class == &(creature_ptr->muta3)) {
        neutralize_base_status(creature_ptr, gm_ptr);
    } else if (gm_ptr->muta_class == &(creature_ptr->muta2)) {
        neutralize_other_status(creature_ptr, gm_ptr);
    }

    creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    set_bits(creature_ptr->update, PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

static void sweep_lose_mutation(player_type *creature_ptr, glm_type *glm_ptr)
{
    int attempts_left = 20;
    if (glm_ptr->choose_mut)
        attempts_left = 1;

    while (attempts_left--) {
        switch_lose_mutation(creature_ptr, glm_ptr);
        if (glm_ptr->muta_class && glm_ptr->muta_which) {
            if (any_bits(*glm_ptr->muta_class, glm_ptr->muta_which)) {
                glm_ptr->muta_chosen = TRUE;
            }
        }

        if (glm_ptr->muta_chosen)
            break;
    }
}

/*!
 * @brief プレイヤーから突然変異を取り除く
 * @param choose_mut 取り除きたい突然変異のID、0ならばランダムに消去
 * @return なし
 */
bool lose_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut)
{
    glm_type tmp_glm;
    glm_type *glm_ptr = initialize_glm_type(&tmp_glm, choose_mut);
    sweep_lose_mutation(creature_ptr, glm_ptr);
    if (!glm_ptr->muta_chosen)
        return FALSE;

    msg_print(glm_ptr->muta_desc);
    if (glm_ptr->muta_class != NULL)
        reset_bits(*glm_ptr->muta_class, (glm_ptr->muta_which));

    set_bits(creature_ptr->update, PU_BONUS);
    handle_stuff(creature_ptr);
    creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    return TRUE;
}

void lose_all_mutations(player_type *creature_ptr)
{
    if (creature_ptr->muta1 || creature_ptr->muta2 || creature_ptr->muta3) {
        chg_virtue(creature_ptr, V_CHANCE, -5);
        msg_print(_("全ての突然変異が治った。", "You are cured of all mutations."));
        creature_ptr->muta1 = creature_ptr->muta2 = creature_ptr->muta3 = 0;
        set_bits(creature_ptr->update, PU_BONUS);
        handle_stuff(creature_ptr);
        creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    }
}
