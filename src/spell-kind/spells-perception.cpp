﻿#include "spell-kind/spells-perception.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/play-record-options.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-slot-types.h"
#include "inventory/player-inventory.h"
#include "io/write-diary.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-perception.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

#include <memory>

/*!
 * @brief 全所持アイテム鑑定処理 /
 * Identify everything being carried.
 * Done by a potion of "self knowledge".
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void identify_pack(PlayerType *player_ptr)
{
    for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        identify_item(player_ptr, o_ptr);
        autopick_alter_item(player_ptr, i, false);
    }
}

/*!
 * @brief アイテム鑑定処理 /
 * Identify an object
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 鑑定されるアイテムの情報参照ポインタ
 * @return 実際に鑑定できたらTRUEを返す
 */
bool identify_item(PlayerType *player_ptr, object_type *o_ptr)
{
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);

    bool old_known = false;
    if (any_bits(o_ptr->ident, IDENT_KNOWN))
        old_known = true;

    if (!o_ptr->is_fully_known()) {
        if (o_ptr->is_artifact() || one_in_(5))
            chg_virtue(player_ptr, V_KNOWLEDGE, 1);
    }

    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);
    set_bits(o_ptr->marked, OM_TOUCHED);

    set_bits(player_ptr->update, PU_BONUS | PU_COMBINE | PU_REORDER);
    set_bits(player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);

    strcpy(record_o_name, o_name);
    record_turn = w_ptr->game_turn;

    describe_flavor(player_ptr, o_name, o_ptr, OD_NAME_ONLY);

    if (record_fix_art && !old_known && o_ptr->is_fixed_artifact())
        exe_write_diary(player_ptr, DIARY_ART, 0, o_name);
    if (record_rand_art && !old_known && o_ptr->art_name)
        exe_write_diary(player_ptr, DIARY_ART, 0, o_name);

    return old_known;
}

/*!
 * @brief アイテム鑑定のメインルーチン処理 /
 * Identify an object in the inventory (or on the floor)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に鑑定を行ったならばTRUEを返す
 * @details
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell(PlayerType *player_ptr, bool only_equip)
{
    std::unique_ptr<ItemTester> item_tester = std::make_unique<FuncItemTester>(only_equip ? object_is_not_identified_weapon_armor : object_is_not_identified);

    concptr q;
    if (can_get_item(player_ptr, *item_tester)) {
        q = _("どのアイテムを鑑定しますか? ", "Identify which item? ");
    } else {
        if (only_equip) {
            item_tester = std::make_unique<FuncItemTester>(&object_type::is_weapon_armour_ammo);
        } else {
            item_tester = std::make_unique<AllMatchItemTester>();
        }
        q = _("すべて鑑定済みです。 ", "All items are identified. ");
    }

    concptr s = _("鑑定するべきアイテムがない。", "You have nothing to identify.");
    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), *item_tester);
    if (!o_ptr)
        return false;

    bool old_known = identify_item(player_ptr, o_ptr);

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);
    if (item >= INVEN_MAIN_HAND) {
        msg_format(_("%^s: %s(%c)。", "%^s: %s (%c)."), describe_use(player_ptr, item), o_name, index_to_label(item));
    } else if (item >= 0) {
        msg_format(_("ザック中: %s(%c)。", "In your pack: %s (%c)."), o_name, index_to_label(item));
    } else {
        msg_format(_("床上: %s。", "On the ground: %s."), o_name);
    }

    autopick_alter_item(player_ptr, item, (bool)(destroy_identify && !old_known));
    return true;
}

/*!
 * @brief アイテム*鑑定*のメインルーチン処理 /
 * Identify an object in the inventory (or on the floor)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に鑑定を行ったならばTRUEを返す
 * @details
 * Fully "identify" an object in the inventory -BEN-
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully(PlayerType *player_ptr, bool only_equip)
{
    std::unique_ptr<ItemTester> item_tester
        = std::make_unique<FuncItemTester>(only_equip ? object_is_not_fully_identified_weapon_armour : object_is_not_fully_identified);

    concptr q;
    if (can_get_item(player_ptr, *item_tester)) {
        q = _("どのアイテムを*鑑定*しますか? ", "*Identify* which item? ");
    } else {
        if (only_equip) {
            item_tester = std::make_unique<FuncItemTester>(&object_type::is_weapon_armour_ammo);
        } else {
            item_tester = std::make_unique<AllMatchItemTester>();
        }
        q = _("すべて*鑑定*済みです。 ", "All items are *identified*. ");
    }

    concptr s = _("*鑑定*するべきアイテムがない。", "You have nothing to *identify*.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), *item_tester);
    if (!o_ptr)
        return false;

    bool old_known = identify_item(player_ptr, o_ptr);

    o_ptr->ident |= (IDENT_FULL_KNOWN);

    /* Refrect item informaiton onto subwindows without updating inventory */
    player_ptr->update &= ~(PU_COMBINE | PU_REORDER);
    handle_stuff(player_ptr);
    player_ptr->update |= (PU_COMBINE | PU_REORDER);

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, 0);
    if (item >= INVEN_MAIN_HAND) {
        msg_format(_("%^s: %s(%c)。", "%^s: %s (%c)."), describe_use(player_ptr, item), o_name, index_to_label(item));
    } else if (item >= 0) {
        msg_format(_("ザック中: %s(%c)。", "In your pack: %s (%c)."), o_name, index_to_label(item));
    } else {
        msg_format(_("床上: %s。", "On the ground: %s."), o_name);
    }

    (void)screen_object(player_ptr, o_ptr, 0L);
    autopick_alter_item(player_ptr, item, (bool)(destroy_identify && !old_known));
    return true;
}
