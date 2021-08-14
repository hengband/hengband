#include "mutation/mutation-investor-remover.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "mutation/gain-mutation-switcher.h"
#include "mutation/lose-mutation-switcher.h"
#include "mutation/mutation-calculator.h" //!< @todo calc_mutant_regenerate_mod() が相互依存している、後で消す.
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-util.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static void sweep_gain_mutation(player_type *creature_ptr, glm_type *gm_ptr)
{
    int attempts_left = 20;
    if (gm_ptr->choose_mut)
        attempts_left = 1;

    while (attempts_left--) {
        switch_gain_mutation(creature_ptr, gm_ptr);
        if (gm_ptr->muta_which != MUTA::MAX && creature_ptr->muta.has_not(gm_ptr->muta_which))
            gm_ptr->muta_chosen = true;

        if (gm_ptr->muta_chosen)
            break;
    }
}

static void race_dependent_mutation(player_type *creature_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->choose_mut != 0)
        return;

    if (creature_ptr->prace == player_race_type::VAMPIRE && creature_ptr->muta.has_not(MUTA::HYPN_GAZE) && (randint1(10) < 7)) {
        gm_ptr->muta_which = MUTA::HYPN_GAZE;
        gm_ptr->muta_desc = _("眼が幻惑的になった...", "Your eyes look mesmerizing...");
        return;
    }

    if (creature_ptr->prace == player_race_type::IMP && creature_ptr->muta.has_not(MUTA::HORNS) && (randint1(10) < 7)) {
        gm_ptr->muta_which = MUTA::HORNS;
        gm_ptr->muta_desc = _("角が額から生えてきた！", "Horns pop forth into your forehead!");
        return;
    }

    if (creature_ptr->prace == player_race_type::YEEK && creature_ptr->muta.has_not(MUTA::SHRIEK) && (randint1(10) < 7)) {
        gm_ptr->muta_which = MUTA::SHRIEK;
        gm_ptr->muta_desc = _("声質がかなり強くなった。", "Your vocal cords get much tougher.");
        return;
    }

    if (creature_ptr->prace == player_race_type::BEASTMAN && creature_ptr->muta.has_not(MUTA::POLYMORPH) && (randint1(10) < 2)) {
        gm_ptr->muta_which = MUTA::POLYMORPH;
        gm_ptr->muta_desc = _("あなたの肉体は変化できるようになった、", "Your body seems mutable.");
        return;
    }

    if (creature_ptr->prace == player_race_type::MIND_FLAYER && creature_ptr->muta.has_not(MUTA::TENTACLES) && (randint1(10) < 7)) {
        gm_ptr->muta_which = MUTA::TENTACLES;
        gm_ptr->muta_desc = _("邪悪な触手が口の周りに生えた。", "Evil-looking tentacles sprout from your mouth.");
    }
}

static void neutralize_base_status(player_type *creature_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == MUTA::PUNY) {
        if (creature_ptr->muta.has(MUTA::HYPER_STR)) {
            msg_print(_("あなたはもう超人的に強くはない！", "You no longer feel super-strong!"));
            creature_ptr->muta.reset(MUTA::HYPER_STR);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::HYPER_STR) {
        if (creature_ptr->muta.has(MUTA::PUNY)) {
            msg_print(_("あなたはもう虚弱ではない！", "You no longer feel puny!"));
            creature_ptr->muta.reset(MUTA::PUNY);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::MORONIC) {
        if (creature_ptr->muta.has(MUTA::HYPER_INT)) {
            msg_print(_("あなたの脳はもう生体コンピュータではない。", "Your brain is no longer a living computer."));
            creature_ptr->muta.reset(MUTA::HYPER_INT);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::HYPER_INT) {
        if (creature_ptr->muta.has(MUTA::MORONIC)) {
            msg_print(_("あなたはもう精神薄弱ではない。", "You are no longer moronic."));
            creature_ptr->muta.reset(MUTA::MORONIC);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::IRON_SKIN) {
        if (creature_ptr->muta.has(MUTA::SCALES)) {
            msg_print(_("鱗がなくなった。", "You lose your scales."));
            creature_ptr->muta.reset(MUTA::SCALES);
        }

        if (creature_ptr->muta.has(MUTA::FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh rots no longer."));
            creature_ptr->muta.reset(MUTA::FLESH_ROT);
        }

        if (creature_ptr->muta.has(MUTA::WART_SKIN)) {
            msg_print(_("肌のイボイボがなくなった。", "You lose your warts."));
            creature_ptr->muta.reset(MUTA::WART_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::WART_SKIN || gm_ptr->muta_which == MUTA::SCALES || gm_ptr->muta_which == MUTA::FLESH_ROT) {
        if (creature_ptr->muta.has(MUTA::IRON_SKIN)) {
            msg_print(_("あなたの肌はもう鉄ではない。", "Your skin is no longer made of steel."));
            creature_ptr->muta.reset(MUTA::IRON_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::FEARLESS) {
        if (creature_ptr->muta.has(MUTA::COWARDICE)) {
            msg_print(_("臆病でなくなった。", "You are no longer cowardly."));
            creature_ptr->muta.reset(MUTA::COWARDICE);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::FLESH_ROT) {
        if (creature_ptr->muta.has(MUTA::REGEN)) {
            msg_print(_("急速に回復しなくなった。", "You stop regenerating."));
            creature_ptr->muta.reset(MUTA::REGEN);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::REGEN) {
        if (creature_ptr->muta.has(MUTA::FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh stops rotting."));
            creature_ptr->muta.reset(MUTA::FLESH_ROT);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::LIMBER) {
        if (creature_ptr->muta.has(MUTA::ARTHRITIS)) {
            msg_print(_("関節が痛くなくなった。", "Your joints stop hurting."));
            creature_ptr->muta.reset(MUTA::ARTHRITIS);
        }

        return;
    }

    if (gm_ptr->muta_which == MUTA::ARTHRITIS) {
        if (creature_ptr->muta.has(MUTA::LIMBER)) {
            msg_print(_("あなたはしなやかでなくなった。", "You no longer feel limber."));
            creature_ptr->muta.reset(MUTA::LIMBER);
        }

        return;
    }
}

static void neutralize_other_status(player_type *creature_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == MUTA::COWARDICE) {
        if (creature_ptr->muta.has(MUTA::FEARLESS)) {
            msg_print(_("恐れ知らずでなくなった。", "You no longer feel fearless."));
            creature_ptr->muta.reset(MUTA::FEARLESS);
        }
    }

    if (gm_ptr->muta_which == MUTA::BEAK) {
        if (creature_ptr->muta.has(MUTA::TRUNK)) {
            msg_print(_("あなたの鼻はもう象の鼻のようではなくなった。", "Your nose is no longer elephantine."));
            creature_ptr->muta.reset(MUTA::TRUNK);
        }
    }

    if (gm_ptr->muta_which == MUTA::TRUNK) {
        if (creature_ptr->muta.has(MUTA::BEAK)) {
            msg_print(_("硬いクチバシがなくなった。", "You no longer have a hard beak."));
            creature_ptr->muta.reset(MUTA::BEAK);
        }
    }
}

/*!
 * @brief プレイヤーに突然変異を与える
 * @param choose_mut 与えたい突然変異のID、0ならばランダムに選択
 */
bool gain_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut)
{
    glm_type tmp_gm;
    glm_type *gm_ptr = initialize_glm_type(&tmp_gm, choose_mut);
    sweep_gain_mutation(creature_ptr, gm_ptr);
    if (!gm_ptr->muta_chosen) {
        msg_print(_("普通になった気がする。", "You feel normal."));
        return false;
    }

    chg_virtue(creature_ptr, V_CHANCE, 1);
    race_dependent_mutation(creature_ptr, gm_ptr);
    msg_print(_("突然変異した！", "You mutate!"));
    msg_print(gm_ptr->muta_desc);
    if (gm_ptr->muta_which != MUTA::MAX)
        creature_ptr->muta.set(gm_ptr->muta_which);

    neutralize_base_status(creature_ptr, gm_ptr);
    neutralize_other_status(creature_ptr, gm_ptr);

    creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    set_bits(creature_ptr->update, PU_BONUS);
    handle_stuff(creature_ptr);
    return true;
}

static void sweep_lose_mutation(player_type *creature_ptr, glm_type *glm_ptr)
{
    int attempts_left = 20;
    if (glm_ptr->choose_mut)
        attempts_left = 1;

    while (attempts_left--) {
        switch_lose_mutation(creature_ptr, glm_ptr);
        if (glm_ptr->muta_which != MUTA::MAX) {
            if (creature_ptr->muta.has(glm_ptr->muta_which)) {
                glm_ptr->muta_chosen = true;
            }
        }

        if (glm_ptr->muta_chosen)
            break;
    }
}

/*!
 * @brief プレイヤーから突然変異を取り除く
 * @param choose_mut 取り除きたい突然変異のID、0ならばランダムに消去
 */
bool lose_mutation(player_type *creature_ptr, MUTATION_IDX choose_mut)
{
    glm_type tmp_glm;
    glm_type *glm_ptr = initialize_glm_type(&tmp_glm, choose_mut);
    sweep_lose_mutation(creature_ptr, glm_ptr);
    if (!glm_ptr->muta_chosen)
        return false;

    msg_print(glm_ptr->muta_desc);
    if (glm_ptr->muta_which != MUTA::MAX)
        creature_ptr->muta.reset(glm_ptr->muta_which);

    set_bits(creature_ptr->update, PU_BONUS);
    handle_stuff(creature_ptr);
    creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    return true;
}

void lose_all_mutations(player_type *creature_ptr)
{
    if (creature_ptr->muta.any()) {
        chg_virtue(creature_ptr, V_CHANCE, -5);
        msg_print(_("全ての突然変異が治った。", "You are cured of all mutations."));
        creature_ptr->muta.clear();
        set_bits(creature_ptr->update, PU_BONUS);
        handle_stuff(creature_ptr);
        creature_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(creature_ptr);
    }
}
