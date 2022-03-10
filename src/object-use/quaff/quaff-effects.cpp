#include "object-use/quaff/quaff-effects.h"
#include "avatar/avatar.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-teleport.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

QuaffEffects::QuaffEffects(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 酔っ払いの薬
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return カオス耐性があるかその他の一部確率でFALSE、それ以外はTRUE
 */
bool QuaffEffects::booze()
{
    auto ident = false;
    auto is_monk = PlayerClass(this->player_ptr).equals(PlayerClassType::MONK);
    if (!is_monk) {
        chg_virtue(this->player_ptr, V_HARMONY, -1);
    } else if (!has_resist_conf(this->player_ptr)) {
        set_bits(this->player_ptr->special_attack, ATTACK_SUIKEN);
    }

    BadStatusSetter bss(this->player_ptr);
    if (!has_resist_conf(this->player_ptr) && bss.confusion(randint0(20) + 15)) {
        ident = true;
    }

    if (has_resist_chaos(this->player_ptr)) {
        return ident;
    }

    if (one_in_(2) && bss.mod_hallucination(randint0(150) + 150)) {
        ident = true;
    }

    if (!is_monk || !one_in_(13)) {
        return ident;
    }

    ident = true;
    if (one_in_(3)) {
        lose_all_info(this->player_ptr);
    } else {
        wiz_dark(this->player_ptr);
    }

    (void)teleport_player_aux(this->player_ptr, 100, false, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    wiz_dark(this->player_ptr);
    msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
    msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing or how you got here!"));
    return ident;
}

/*!
 * @brief 爆発の薬の効果処理 / Fumble ramble
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool QuaffEffects::detonation()
{
    msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
    take_hit(this->player_ptr, DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"));
    BadStatusSetter bss(this->player_ptr);
    (void)bss.mod_stun(75);
    (void)bss.mod_cut(5000);
    return true;
}
