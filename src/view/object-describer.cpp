#include "view/object-describer.h"
#include "cmd-action/cmd-spell.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "object-enchant/special-object-flags.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "spell/spell-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "tracking/baseitem-tracker.h"
#include "view/display-messages.h"

/*!
 * @brief 魔道具の使用回数の残量を示すメッセージを表示する
 * @param item 残量を表示したいインベントリ内のアイテム
 */
void inven_item_charges(const ItemEntity &item)
{
    if (!item.is_wand_staff() || !item.is_known()) {
        return;
    }

#ifdef JP
    if (item.pval <= 0) {
        msg_print("もう魔力が残っていない。");
    } else {
        msg_format("あと %d 回分の魔力が残っている。", item.pval);
    }
#else
    if (item.pval != 1) {
        msg_format("You have %d charges remaining.", item.pval);
    } else {
        msg_format("You have %d charge remaining.", item.pval);
    }
#endif
}

/*!
 * @brief アイテムの残り所持数メッセージを表示する /
 * Describe an item in the inventory.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 残量を表示したいプレイヤーのアイテム所持スロット
 */
void inven_item_describe(PlayerType *player_ptr, short i_idx)
{
    const auto &item = player_ptr->inventory_list[i_idx];
    const auto item_name = describe_flavor(player_ptr, item, 0);
#ifdef JP
    if (item.number <= 0) {
        msg_format("もう%sを持っていない。", item_name.data());
    } else {
        msg_format("まだ %sを持っている。", item_name.data());
    }
#else
    msg_format("You have %s.", item_name.data());
#endif
}

/*!
 * @brief 現在アクティブになっているウィンドウにオブジェクトの詳細を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details Include list of usable spells for readible books
 */
void display_koff(PlayerType *player_ptr)
{
    const auto &tracker = BaseitemTracker::get_instance();
    if (!tracker.is_tracking()) {
        return;
    }

    for (auto y = 0; y < game_term->hgt; y++) {
        term_erase(0, y);
    }

    const auto item = tracker.get_trackee();
    const auto item_name = describe_flavor(player_ptr, item, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));
    term_putstr(0, 0, -1, TERM_WHITE, item_name);
    const auto sval = *item.bi_key.sval();
    const auto use_realm = PlayerRealm::get_realm_of_book(item.bi_key.tval());

    PlayerRealm pr(player_ptr);
    if (pr.realm1().is_available() || pr.realm2().is_available()) {
        if (!pr.realm1().equals(use_realm) && !pr.realm2().equals(use_realm)) {
            return;
        }
    } else {
        PlayerClass pc(player_ptr);
        if (!pc.is_every_magic()) {
            return;
        }
        if (!PlayerRealm::is_magic(use_realm)) {
            return;
        }
        if (pc.equals(PlayerClassType::RED_MAGE) && (use_realm != RealmType::ARCANE) && (sval > 1)) {
            return;
        }
    }

    auto num = 0;
    int spells[64]{};
    for (int spell = 0; spell < 32; spell++) {
        if (fake_spell_flags[sval] & (1UL << spell)) {
            spells[num++] = spell;
        }
    }

    print_spells(player_ptr, 0, spells, num, 2, 0, use_realm);
}
