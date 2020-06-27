#include "mind/mind-force-trainer.h"
#include "cmd-action/cmd-pet.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "player/avatar.h"
#include "player/player-move.h"
#include "view/display-messages.h"

/*!
 * @brief 練気術師が「練気」で溜めた気の量を返す
 * @param caster_ptr プレーヤーの参照ポインタ
 * @return 現在溜まっている気の量
 */
MAGIC_NUM1 get_current_ki(player_type *caster_ptr)
{
    return caster_ptr->magic_num1[0];
}

/*!
 * @brief 練気術師において、気を溜める
 * @param caster_ptr プレーヤーの参照ポインタ
 * @param is_reset TRUEなら気の量をkiにセットし、FALSEなら加減算を行う
 * @param ki 気の量
 * @return なし
 */
void set_current_ki(player_type *caster_ptr, bool is_reset, MAGIC_NUM1 ki)
{
    if (is_reset) {
        caster_ptr->magic_num1[0] = ki;
        return;
    }

    caster_ptr->magic_num1[0] += ki;
}

bool clear_mind(player_type *creature_ptr)
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return FALSE;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

    creature_ptr->csp += (3 + creature_ptr->lev / 20);
    if (creature_ptr->csp >= creature_ptr->msp) {
        creature_ptr->csp = creature_ptr->msp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= (PR_MANA);
    return TRUE;
}

/*!
 * @brief 光速移動の継続時間をセットする / Set "lightspeed", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return なし
 */
void set_lightspeed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return;

    if (creature_ptr->wild_mode)
        v = 0;

    if (v) {
        if (creature_ptr->lightspeed && !do_dec) {
            if (creature_ptr->lightspeed > v)
                return;
        } else if (!creature_ptr->lightspeed) {
            msg_print(_("非常に素早く動けるようになった！", "You feel yourself moving extremely fast!"));
            notice = TRUE;
            chg_virtue(creature_ptr, V_PATIENCE, -1);
            chg_virtue(creature_ptr, V_DILIGENCE, 1);
        }
    } else {
        if (creature_ptr->lightspeed) {
            msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            notice = TRUE;
        }
    }

    creature_ptr->lightspeed = v;

    if (!notice)
        return;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
}
