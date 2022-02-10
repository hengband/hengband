#include "spell/spells-staff-only.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "hpmp/hp-mp-processor.h"
#include "player-base/player-class.h"
#include "player/player-damage.h"
#include "spell-kind/spells-sight.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 聖浄の杖の効果
 * @param player_ptr プレイヤーへの参照ポインタ
 * @magic 魔法の効果である場合TRUE (杖と同じ効果の呪文はあったか？ 要調査)
 * @powerful 効果が増強される時TRUE (TRUEになるタイミングはあるか？ 要調査)
 */
bool cleansing_nova(PlayerType *player_ptr, bool magic, bool powerful)
{
    bool ident = false;
    if (dispel_evil(player_ptr, powerful ? 225 : 150)) {
        ident = true;
    }

    int k = 3 * player_ptr->lev;
    if (set_protevil(player_ptr, (magic ? 0 : player_ptr->protevil) + randint1(25) + k, false)) {
        ident = true;
    }

    BadStatusSetter bss(player_ptr);
    if (bss.poison(0)) {
        ident = true;
    }

    if (bss.afraidness(0)) {
        ident = true;
    }

    if (hp_player(player_ptr, 50)) {
        ident = true;
    }

    if (bss.stun(0)) {
        ident = true;
    }

    if (bss.cut(0)) {
        ident = true;
    }

    return ident;
}

/*!
 * @brief 魔力の嵐の杖の効果
 * @param player_ptr プレイヤーへの参照ポインタ
 * @powerful 効果が増強される時TRUE (TRUEになるタイミングはあるか？ 要調査)
 */
bool unleash_mana_storm(PlayerType *player_ptr, bool powerful)
{
    msg_print(_("強力な魔力が敵を引き裂いた！", "Mighty magics rend your enemies!"));
    project(player_ptr, 0, (powerful ? 7 : 5), player_ptr->y, player_ptr->x, (randint1(200) + (powerful ? 500 : 300)) * 2, AttributeType::MANA,
        PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID);

    if (!PlayerClass(player_ptr).is_wizard())
        (void)take_hit(player_ptr, DAMAGE_NOESCAPE, 50, _("コントロールし難い強力な魔力の解放", "unleashing magics too mighty to control"));

    return true;
}
