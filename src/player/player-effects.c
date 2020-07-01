/*!
 * @brief プレイヤーのステータス管理 / effects of various "objects"
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

#include "player/player-effects.h"
#include "autopick/autopick-reader-writer.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-stat.h"
#include "birth/character-builder.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "core/hp-mp-processor.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "io/input-key-acceptor.h"
#include "io/report.h"
#include "io/save.h"
#include "locale/vowel-checker.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-sniper.h"
#include "mind/stances-table.h"
#include "monster/monster-status.h"
#include "mutation/mutation.h"
#include "object-enchant/artifact.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "object/object-value-calc.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-personalities-types.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "realm/realm-song-numbers.h"
#include "spell-kind/spells-floor.h"
#include "spell-realm/spells-craft.h"
#include "spell-realm/spells-crusade.h"
#include "spell-realm/spells-demon.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/form-changer.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの継続行動を設定する。
 * @param typ 継続行動のID\n
 * #ACTION_NONE / #ACTION_SEARCH / #ACTION_REST / #ACTION_LEARN / #ACTION_FISH / #ACTION_KAMAE / #ACTION_KATA / #ACTION_SING / #ACTION_HAYAGAKE / #ACTION_SPELL
 * から選択。
 * @return なし
 */
void set_action(player_type *creature_ptr, ACTION_IDX typ)
{
    int prev_typ = creature_ptr->action;

    if (typ == prev_typ) {
        return;
    } else {
        switch (prev_typ) {
        case ACTION_SEARCH: {
            msg_print(_("探索をやめた。", "You no longer walk carefully."));
            creature_ptr->redraw |= (PR_SPEED);
            break;
        }
        case ACTION_REST: {
            creature_ptr->resting = 0;
            break;
        }
        case ACTION_LEARN: {
            msg_print(_("学習をやめた。", "You stop learning."));
            creature_ptr->new_mane = FALSE;
            break;
        }
        case ACTION_KAMAE: {
            msg_print(_("構えをといた。", "You stop assuming the special stance."));
            creature_ptr->special_defense &= ~(KAMAE_MASK);
            break;
        }
        case ACTION_KATA: {
            msg_print(_("型を崩した。", "You stop assuming the special stance."));
            creature_ptr->special_defense &= ~(KATA_MASK);
            creature_ptr->update |= (PU_MONSTERS);
            creature_ptr->redraw |= (PR_STATUS);
            break;
        }
        case ACTION_SING: {
            msg_print(_("歌うのをやめた。", "You stop singing."));
            break;
        }
        case ACTION_HAYAGAKE: {
            msg_print(_("足が重くなった。", "You are no longer walking extremely fast."));
            take_turn(creature_ptr, 100);
            break;
        }
        case ACTION_SPELL: {
            msg_print(_("呪文の詠唱を中断した。", "You stopped casting."));
            break;
        }
        }
    }

    creature_ptr->action = typ;

    /* If we are requested other action, stop singing */
    if (prev_typ == ACTION_SING)
        stop_singing(creature_ptr);
    if (prev_typ == ACTION_SPELL)
        stop_hex_spell(creature_ptr);

    switch (creature_ptr->action) {
    case ACTION_SEARCH: {
        msg_print(_("注意深く歩き始めた。", "You begin to walk carefully."));
        creature_ptr->redraw |= (PR_SPEED);
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
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->redraw |= (PR_STATE);
}
