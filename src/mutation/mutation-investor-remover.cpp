#include "mutation/mutation-investor-remover.h"
#include "avatar/avatar.h"
#include "core/stuff-handler.h"
#include "mutation/mutation-calculator.h" //!< @todo calc_mutant_regenerate_mod() が相互依存している、後で消す.
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-roll-table.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <string_view>

namespace {
//! 狂戦士は抽選値がこの値より大きい突然変異だけを得る
constexpr int BERSERKER_ROLL_OFFSET = 74;

//! 突然変異をランダムに選ぶときの試行回数
constexpr int MUTATION_SELECTION_ATTEMPTS = 20;

struct glm_type {
    explicit glm_type(MUTATION_IDX roll)
        : choose_mut(roll)
    {
    }

    PlayerMutationType muta_which = PlayerMutationType::MAX;
    std::string_view muta_desc;
    bool muta_chosen = false;
    MUTATION_IDX choose_mut;
};
}

/*!
 * @brief 獲得する突然変異を抽選する
 * @param choose_mut 抽選値。0ならばランダムに決める
 * @return 抽選した突然変異。抽選値に対応する突然変異が無いか、プレイヤーが得られない突然変異ならnullopt
 */
static tl::optional<const MutationRollEntry &> select_gain_mutation(PlayerType *player_ptr, MUTATION_IDX choose_mut)
{
    const PlayerClass pc(player_ptr);
    const auto roll = [&] {
        if (choose_mut) {
            return choose_mut;
        }

        return pc.equals(PlayerClassType::BERSERKER) ? BERSERKER_ROLL_OFFSET + randint1(MUTATION_ROLL_MAX - BERSERKER_ROLL_OFFSET) : randint1(MUTATION_ROLL_MAX);
    }();

    const auto entry = find_mutation_by_roll(roll);
    if (!entry) {
        return tl::nullopt;
    }

    if ((entry->type == PlayerMutationType::CHAOS_GIFT) && pc.equals(PlayerClassType::CHAOS_WARRIOR)) {
        return tl::nullopt;
    }

    if ((entry->type == PlayerMutationType::BAD_LUCK) && (player_ptr->ppersonality == PERSONALITY_LUCKY)) {
        return tl::nullopt;
    }

    return entry;
}

static void sweep_gain_mutation(PlayerType *player_ptr, glm_type *gm_ptr)
{
    const auto attempts = gm_ptr->choose_mut ? 1 : MUTATION_SELECTION_ATTEMPTS;
    for (auto i = 0; i < attempts; i++) {
        const auto entry = select_gain_mutation(player_ptr, gm_ptr->choose_mut);
        if (entry && player_ptr->muta.has_not(entry->type)) {
            gm_ptr->muta_which = entry->type;
            gm_ptr->muta_desc = entry->gain_message;
            gm_ptr->muta_chosen = true;
            return;
        }
    }
}

static void race_dependent_mutation(PlayerType *player_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->choose_mut != 0) {
        return;
    }

    PlayerRace pr(player_ptr);
    if (pr.equals(PlayerRaceType::VAMPIRE) && player_ptr->muta.has_not(PlayerMutationType::HYPN_GAZE) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::HYPN_GAZE;
        gm_ptr->muta_desc = _("眼が幻惑的になった...", "Your eyes look mesmerizing...");
        return;
    }

    if (pr.equals(PlayerRaceType::IMP) && player_ptr->muta.has_not(PlayerMutationType::HORNS) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::HORNS;
        gm_ptr->muta_desc = _("角が額から生えてきた！", "Horns pop forth into your forehead!");
        return;
    }

    if (pr.equals(PlayerRaceType::YEEK) && player_ptr->muta.has_not(PlayerMutationType::SHRIEK) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::SHRIEK;
        gm_ptr->muta_desc = _("声質がかなり強くなった。", "Your vocal cords get much tougher.");
        return;
    }

    if (pr.equals(PlayerRaceType::BEASTMAN) && player_ptr->muta.has_not(PlayerMutationType::POLYMORPH) && (randint1(10) < 2)) {
        gm_ptr->muta_which = PlayerMutationType::POLYMORPH;
        gm_ptr->muta_desc = _("あなたの肉体は変化できるようになった、", "Your body seems mutable.");
        return;
    }

    if (pr.equals(PlayerRaceType::MIND_FLAYER) && player_ptr->muta.has_not(PlayerMutationType::TENTACLES) && (randint1(10) < 7)) {
        gm_ptr->muta_which = PlayerMutationType::TENTACLES;
        gm_ptr->muta_desc = _("邪悪な触手が口の周りに生えた。", "Evil-looking tentacles sprout from your mouth.");
    }
}

static void neutralize_base_status(PlayerType *player_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == PlayerMutationType::PUNY) {
        if (player_ptr->muta.has(PlayerMutationType::HYPER_STR)) {
            msg_print(_("あなたはもう超人的に強くはない！", "You no longer feel super-strong!"));
            player_ptr->muta.reset(PlayerMutationType::HYPER_STR);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::HYPER_STR) {
        if (player_ptr->muta.has(PlayerMutationType::PUNY)) {
            msg_print(_("あなたはもう虚弱ではない！", "You no longer feel puny!"));
            player_ptr->muta.reset(PlayerMutationType::PUNY);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::MORONIC) {
        if (player_ptr->muta.has(PlayerMutationType::HYPER_INT)) {
            msg_print(_("あなたの脳はもう生体コンピュータではない。", "Your brain is no longer a living computer."));
            player_ptr->muta.reset(PlayerMutationType::HYPER_INT);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::HYPER_INT) {
        if (player_ptr->muta.has(PlayerMutationType::MORONIC)) {
            msg_print(_("あなたはもう精神薄弱ではない。", "You are no longer moronic."));
            player_ptr->muta.reset(PlayerMutationType::MORONIC);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::IRON_SKIN) {
        if (player_ptr->muta.has(PlayerMutationType::SCALES)) {
            msg_print(_("鱗がなくなった。", "You lose your scales."));
            player_ptr->muta.reset(PlayerMutationType::SCALES);
        }

        if (player_ptr->muta.has(PlayerMutationType::FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh rots no longer."));
            player_ptr->muta.reset(PlayerMutationType::FLESH_ROT);
        }

        if (player_ptr->muta.has(PlayerMutationType::WART_SKIN)) {
            msg_print(_("肌のイボイボがなくなった。", "You lose your warts."));
            player_ptr->muta.reset(PlayerMutationType::WART_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::WART_SKIN || gm_ptr->muta_which == PlayerMutationType::SCALES || gm_ptr->muta_which == PlayerMutationType::FLESH_ROT) {
        if (player_ptr->muta.has(PlayerMutationType::IRON_SKIN)) {
            msg_print(_("あなたの肌はもう鉄ではない。", "Your skin is no longer made of steel."));
            player_ptr->muta.reset(PlayerMutationType::IRON_SKIN);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::FEARLESS) {
        if (player_ptr->muta.has(PlayerMutationType::COWARDICE)) {
            msg_print(_("臆病でなくなった。", "You are no longer cowardly."));
            player_ptr->muta.reset(PlayerMutationType::COWARDICE);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::FLESH_ROT) {
        if (player_ptr->muta.has(PlayerMutationType::REGEN)) {
            msg_print(_("急速に回復しなくなった。", "You stop regenerating."));
            player_ptr->muta.reset(PlayerMutationType::REGEN);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::REGEN) {
        if (player_ptr->muta.has(PlayerMutationType::FLESH_ROT)) {
            msg_print(_("肉体が腐乱しなくなった。", "Your flesh stops rotting."));
            player_ptr->muta.reset(PlayerMutationType::FLESH_ROT);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::LIMBER) {
        if (player_ptr->muta.has(PlayerMutationType::ARTHRITIS)) {
            msg_print(_("関節が痛くなくなった。", "Your joints stop hurting."));
            player_ptr->muta.reset(PlayerMutationType::ARTHRITIS);
        }

        return;
    }

    if (gm_ptr->muta_which == PlayerMutationType::ARTHRITIS) {
        if (player_ptr->muta.has(PlayerMutationType::LIMBER)) {
            msg_print(_("あなたはしなやかでなくなった。", "You no longer feel limber."));
            player_ptr->muta.reset(PlayerMutationType::LIMBER);
        }

        return;
    }
}

static void neutralize_other_status(PlayerType *player_ptr, glm_type *gm_ptr)
{
    if (gm_ptr->muta_which == PlayerMutationType::COWARDICE) {
        if (player_ptr->muta.has(PlayerMutationType::FEARLESS)) {
            msg_print(_("恐れ知らずでなくなった。", "You no longer feel fearless."));
            player_ptr->muta.reset(PlayerMutationType::FEARLESS);
        }
    }

    if (gm_ptr->muta_which == PlayerMutationType::BEAK) {
        if (player_ptr->muta.has(PlayerMutationType::TRUNK)) {
            msg_print(_("あなたの鼻はもう象の鼻のようではなくなった。", "Your nose is no longer elephantine."));
            player_ptr->muta.reset(PlayerMutationType::TRUNK);
        }
    }

    if (gm_ptr->muta_which == PlayerMutationType::TRUNK) {
        if (player_ptr->muta.has(PlayerMutationType::BEAK)) {
            msg_print(_("硬いクチバシがなくなった。", "You no longer have a hard beak."));
            player_ptr->muta.reset(PlayerMutationType::BEAK);
        }
    }
}

/*!
 * @brief プレイヤーに突然変異を与える
 * @param choose_mut 与えたい突然変異のID、0ならばランダムに選択
 */
bool gain_mutation(PlayerType *player_ptr, MUTATION_IDX choose_mut)
{
    glm_type gm(choose_mut);
    sweep_gain_mutation(player_ptr, &gm);
    if (!gm.muta_chosen) {
        msg_print(_("普通になった気がする。", "You feel normal."));
        return false;
    }

    chg_virtue(player_ptr, Virtue::CHANCE, 1);
    race_dependent_mutation(player_ptr, &gm);
    msg_print(_("突然変異した！", "You mutate!"));
    msg_print(gm.muta_desc);
    player_ptr->muta.set(gm.muta_which);

    neutralize_base_status(player_ptr, &gm);
    neutralize_other_status(player_ptr, &gm);

    player_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(player_ptr);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 失う突然変異を抽選する
 * @param choose_mut 抽選値。0ならばランダムに決める
 * @return 抽選した突然変異。抽選値に対応する突然変異が無いか、プレイヤーが失わない突然変異ならnullopt
 */
static tl::optional<const MutationRollEntry &> select_lose_mutation(PlayerType *player_ptr, MUTATION_IDX choose_mut)
{
    const auto roll = choose_mut ? choose_mut : randint1(MUTATION_ROLL_MAX);
    const auto entry = find_mutation_by_roll(roll);
    if (!entry) {
        return tl::nullopt;
    }

    if ((entry->type == PlayerMutationType::GOOD_LUCK) && (player_ptr->ppersonality == PERSONALITY_LUCKY)) {
        return tl::nullopt;
    }

    return entry;
}

static void sweep_lose_mutation(PlayerType *player_ptr, glm_type *glm_ptr)
{
    const auto attempts = glm_ptr->choose_mut ? 1 : MUTATION_SELECTION_ATTEMPTS;
    for (auto i = 0; i < attempts; i++) {
        const auto entry = select_lose_mutation(player_ptr, glm_ptr->choose_mut);
        if (entry && player_ptr->muta.has(entry->type)) {
            glm_ptr->muta_which = entry->type;
            glm_ptr->muta_desc = entry->lose_message;
            glm_ptr->muta_chosen = true;
            return;
        }
    }
}

/*!
 * @brief プレイヤーから突然変異を取り除く
 * @param choose_mut 取り除きたい突然変異のID、0ならばランダムに消去
 */
bool lose_mutation(PlayerType *player_ptr, MUTATION_IDX choose_mut)
{
    glm_type glm(choose_mut);
    sweep_lose_mutation(player_ptr, &glm);
    if (!glm.muta_chosen) {
        return false;
    }

    msg_print(glm.muta_desc);
    player_ptr->muta.reset(glm.muta_which);

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    player_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(player_ptr);
    return true;
}

void lose_all_mutations(PlayerType *player_ptr)
{
    if (player_ptr->muta.any()) {
        chg_virtue(player_ptr, Virtue::CHANCE, -5);
        msg_print(_("全ての突然変異が治った。", "You are cured of all mutations."));
        player_ptr->muta.clear();
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
        handle_stuff(player_ptr);
        player_ptr->mutant_regenerate_mod = calc_mutant_regenerate_mod(player_ptr);
    }
}
