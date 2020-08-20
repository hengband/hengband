#include "spell/spells-staff-only.h"
#include "core/hp-mp-processor.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "player/player-damage.h"
#include "spell-kind/spells-sight.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "view/display-messages.h"

/*!
 * @brief 聖浄の杖の効果
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @magic 魔法の効果である場合TRUE (杖と同じ効果の呪文はあったか？ 要調査)
 * @powerful 効果が増強される時TRUE (TRUEになるタイミングはあるか？ 要調査)
 */
bool cleansing_nova(player_type *creature_ptr, bool magic, bool powerful)
{
    bool ident = FALSE;
    if (dispel_evil(creature_ptr, powerful ? 225 : 150))
        ident = TRUE;

    int k = 3 * creature_ptr->lev;
    if (set_protevil(creature_ptr, (magic ? 0 : creature_ptr->protevil) + randint1(25) + k, FALSE))
        ident = TRUE;

    if (set_poisoned(creature_ptr, 0))
        ident = TRUE;

    if (set_afraid(creature_ptr, 0))
        ident = TRUE;

    if (hp_player(creature_ptr, 50))
        ident = TRUE;

    if (set_stun(creature_ptr, 0))
        ident = TRUE;

    if (set_cut(creature_ptr, 0))
        ident = TRUE;

    return ident;
}

/*!
 * @brief 魔力の嵐の杖の効果
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @powerful 効果が増強される時TRUE (TRUEになるタイミングはあるか？ 要調査)
 */
bool unleash_mana_storm(player_type *creature_ptr, bool powerful)
{
    msg_print(_("強力な魔力が敵を引き裂いた！", "Mighty magics rend your enemies!"));
    project(creature_ptr, 0, (powerful ? 7 : 5), creature_ptr->y, creature_ptr->x, (randint1(200) + (powerful ? 500 : 300)) * 2, GF_MANA,
        PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);

    bool is_special_class = creature_ptr->pclass != CLASS_MAGE;
    is_special_class &= creature_ptr->pclass != CLASS_HIGH_MAGE;
    is_special_class &= creature_ptr->pclass != CLASS_SORCERER;
    is_special_class &= creature_ptr->pclass != CLASS_MAGIC_EATER;
    is_special_class &= creature_ptr->pclass != CLASS_BLUE_MAGE;
    if (is_special_class)
        (void)take_hit(creature_ptr, DAMAGE_NOESCAPE, 50, _("コントロールし難い強力な魔力の解放", "unleashing magics too mighty to control"), -1);

    return TRUE;
}
