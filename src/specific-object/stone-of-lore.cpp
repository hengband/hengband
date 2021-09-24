/*!
 * @brief 知識の石を発動させる処理
 * @date 2021/09/24
 * @author Hourier
 */

#include "specific-object/stone-of-lore.h"
#include "core/player-redraw-types.h"
#include "player/player-damage.h"
#include "spell-kind/spells-perception.h"
#include "status/bad-status-setter.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

StoneOfLore::StoneOfLore(player_type *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 知識の石の発動を実行する / Do activation of the stone of lore.
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @return 実行したらTRUE、しなかったらFALSE
 * @details
 * 鑑定を実行した後HPを消費する。1/5で混乱し、1/20で追加ダメージ。
 * MPがある場合はさらにMPを20消費する。不足する場合は麻痺及び混乱。
 */
bool StoneOfLore::perilous_secrets()
{
    msg_print(_("石が隠された秘密を写し出した．．．", "The stone reveals hidden mysteries..."));
    if (!ident_spell(this->player_ptr, false)) {
        return false;
    }

    BadStatusSetter bss(this->player_ptr);
    if (this->player_ptr->msp > 0) {
        if (20 <= this->player_ptr->csp) {
            this->player_ptr->csp -= 20;
        } else {
            auto oops = 20 - this->player_ptr->csp;
            this->player_ptr->csp = 0;
            this->player_ptr->csp_frac = 0;
            msg_print(_("石を制御できない！", "You are too weak to control the stone!"));
            (void)bss.mod_paralysis(randint1(5 * oops + 1));
            (void)bss.mod_confusion(randint1(5 * oops + 1));
        }

        this->player_ptr->redraw |= PR_MANA;
    }

    take_hit(this->player_ptr, DAMAGE_LOSELIFE, damroll(1, 12), _("危険な秘密", "perilous secrets"));
    if (one_in_(5)) {
        (void)bss.mod_confusion(randint1(10));
    }

    if (one_in_(20)) {
        take_hit(this->player_ptr, DAMAGE_LOSELIFE, damroll(4, 10), _("危険な秘密", "perilous secrets"));
    }

    return true;
}
