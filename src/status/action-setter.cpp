/*!
 * @brief プレイヤーの継続行動処理
 * @date 2014/01/01
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "status/action-setter.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの継続行動を設定する。
 * @param typ 継続行動のID
 * NONE / SEARCH / REST / LEARN / FISH / MONK_STANCE / SAMURAI_STANCE / SING / HAYAGAKE / SPELL
 * から選択。
 */
void set_action(PlayerType *player_ptr, uint8_t typ)
{
    int prev_typ = player_ptr->action;
    if (typ == prev_typ) {
        return;
    }

    switch (prev_typ) {
    case ACTION_SEARCH: {
        msg_print(_("探索をやめた。", "You no longer walk carefully."));
        player_ptr->redraw |= (PR_SPEED);
        break;
    }
    case ACTION_REST: {
        player_ptr->resting = 0;
        break;
    }
    case ACTION_LEARN: {
        msg_print(_("学習をやめた。", "You stop learning."));
        auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
        bluemage_data->new_magic_learned = false;
        break;
    }
    case ACTION_MONK_STANCE: {
        msg_print(_("構えをといた。", "You stop assuming the special stance."));
        PlayerClass(player_ptr).set_monk_stance(MonkStanceType::NONE);
        break;
    }
    case ACTION_SAMURAI_STANCE: {
        msg_print(_("型を崩した。", "You stop assuming the special stance."));
        PlayerClass(player_ptr).set_samurai_stance(SamuraiStanceType::NONE);
        player_ptr->update |= (PU_MONSTER_STATUSES);
        player_ptr->redraw |= (PR_TIMED_EFFECT);
        break;
    }
    case ACTION_SING: {
        msg_print(_("歌うのをやめた。", "You stop singing."));
        break;
    }
    case ACTION_HAYAGAKE: {
        msg_print(_("足が重くなった。", "You are no longer walking extremely fast."));
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        break;
    }
    case ACTION_SPELL: {
        msg_print(_("呪文の詠唱を中断した。", "You stopped casting."));
        break;
    }
    }

    player_ptr->action = typ;

    /* If we are requested other action, stop singing */
    if (prev_typ == ACTION_SING) {
        stop_singing(player_ptr);
    }

    if (prev_typ == ACTION_SPELL) {
        SpellHex spell_hex(player_ptr);
        if (spell_hex.is_spelling_any()) {
            spell_hex.stop_all_spells();
        }
    }

    switch (player_ptr->action) {
    case ACTION_SEARCH: {
        msg_print(_("注意深く歩き始めた。", "You begin to walk carefully."));
        player_ptr->redraw |= (PR_SPEED);
        break;
    }
    case ACTION_LEARN: {
        msg_print(_("学習を始めた。", "You begin learning"));
        break;
    }
    case ACTION_FISH: {
        msg_print(_("水面に糸を垂らした．．．", "You begin fishing..."));
        break;
    }
    case ACTION_HAYAGAKE: {
        msg_print(_("足が羽のように軽くなった。", "You begin to walk extremely fast."));
        break;
    }
    default: {
        break;
    }
    }

    player_ptr->update |= (PU_BONUS);
    player_ptr->redraw |= (PR_ACTION);
}
