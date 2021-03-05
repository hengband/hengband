﻿#include "spell-kind/spells-perception.h"
#include "autopick/autopick.h"
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
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-perception.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player-info/avatar.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 全所持アイテム鑑定処理 /
 * Identify everything being carried.
 * Done by a potion of "self knowledge".
 * @param target_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void identify_pack(player_type *target_ptr)
{
    for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &target_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        identify_item(target_ptr, o_ptr);
        autopick_alter_item(target_ptr, i, FALSE);
    }
}

/*!
 * @brief アイテム鑑定処理 /
 * Identify an object
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 鑑定されるアイテムの情報参照ポインタ
 * @return 実際に鑑定できたらTRUEを返す
 */
bool identify_item(player_type *owner_ptr, object_type *o_ptr)
{
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(owner_ptr, o_name, o_ptr, 0);

    bool old_known = FALSE;
    if (o_ptr->ident & IDENT_KNOWN)
        old_known = TRUE;

    if (!object_is_fully_known(o_ptr)) {
        if (object_is_artifact(o_ptr) || one_in_(5))
            chg_virtue(owner_ptr, V_KNOWLEDGE, 1);
    }

    object_aware(owner_ptr, o_ptr);
    object_known(o_ptr);
    o_ptr->marked |= OM_TOUCHED;

    owner_ptr->update |= (PU_BONUS | PU_COMBINE | PU_REORDER);
    owner_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    strcpy(record_o_name, o_name);
    record_turn = current_world_ptr->game_turn;

    describe_flavor(owner_ptr, o_name, o_ptr, OD_NAME_ONLY);

    if (record_fix_art && !old_known && object_is_fixed_artifact(o_ptr))
        exe_write_diary(owner_ptr, DIARY_ART, 0, o_name);
    if (record_rand_art && !old_known && o_ptr->art_name)
        exe_write_diary(owner_ptr, DIARY_ART, 0, o_name);

    return old_known;
}

/*!
 * @brief アイテム鑑定のメインルーチン処理 /
 * Identify an object in the inventory (or on the floor)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に鑑定を行ったならばTRUEを返す
 * @details
 * This routine does *not* automatically combine objects.
 * Returns TRUE if something was identified, else FALSE.
 */
bool ident_spell(player_type *caster_ptr, bool only_equip, tval_type item_tester_tval)
{
    if (only_equip)
        item_tester_hook = item_tester_hook_identify_weapon_armour;
    else
        item_tester_hook = item_tester_hook_identify;

    concptr q;
    if (can_get_item(caster_ptr, item_tester_tval)) {
        q = _("どのアイテムを鑑定しますか? ", "Identify which item? ");
    } else {
        if (only_equip)
            item_tester_hook = object_is_weapon_armour_ammo;
        else
            item_tester_hook = NULL;

        q = _("すべて鑑定済みです。 ", "All items are identified. ");
    }

    concptr s = _("鑑定するべきアイテムがない。", "You have nothing to identify.");
    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), TV_NONE);
    if (!o_ptr)
        return FALSE;

    bool old_known = identify_item(caster_ptr, o_ptr);

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, 0);
    if (item >= INVEN_MAIN_HAND) {
        msg_format(_("%^s: %s(%c)。", "%^s: %s (%c)."), describe_use(caster_ptr, item), o_name, index_to_label(item));
    } else if (item >= 0) {
        msg_format(_("ザック中: %s(%c)。", "In your pack: %s (%c)."), o_name, index_to_label(item));
    } else {
        msg_format(_("床上: %s。", "On the ground: %s."), o_name);
    }

    autopick_alter_item(caster_ptr, item, (bool)(destroy_identify && !old_known));
    return TRUE;
}

/*!
 * @brief アイテム*鑑定*のメインルーチン処理 /
 * Identify an object in the inventory (or on the floor)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param only_equip 装備品のみを対象とするならばTRUEを返す
 * @return 実際に鑑定を行ったならばTRUEを返す
 * @details
 * Fully "identify" an object in the inventory -BEN-
 * This routine returns TRUE if an item was identified.
 */
bool identify_fully(player_type *caster_ptr, bool only_equip, tval_type item_tester_tval)
{
    if (only_equip)
        item_tester_hook = item_tester_hook_identify_fully_weapon_armour;
    else
        item_tester_hook = item_tester_hook_identify_fully;

    concptr q;
    if (can_get_item(caster_ptr, item_tester_tval)) {
        q = _("どのアイテムを*鑑定*しますか? ", "*Identify* which item? ");
    } else {
        if (only_equip)
            item_tester_hook = object_is_weapon_armour_ammo;
        else
            item_tester_hook = NULL;

        q = _("すべて*鑑定*済みです。 ", "All items are *identified*. ");
    }

    concptr s = _("*鑑定*するべきアイテムがない。", "You have nothing to *identify*.");

    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), TV_NONE);
    if (!o_ptr)
        return FALSE;

    bool old_known = identify_item(caster_ptr, o_ptr);

    o_ptr->ident |= (IDENT_FULL_KNOWN);

    /* Refrect item informaiton onto subwindows without updating inventory */
    caster_ptr->update &= ~(PU_COMBINE | PU_REORDER);
    handle_stuff(caster_ptr);
    caster_ptr->update |= (PU_COMBINE | PU_REORDER);

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, 0);
    if (item >= INVEN_MAIN_HAND) {
        msg_format(_("%^s: %s(%c)。", "%^s: %s (%c)."), describe_use(caster_ptr, item), o_name, index_to_label(item));
    } else if (item >= 0) {
        msg_format(_("ザック中: %s(%c)。", "In your pack: %s (%c)."), o_name, index_to_label(item));
    } else {
        msg_format(_("床上: %s。", "On the ground: %s."), o_name);
    }

    (void)screen_object(caster_ptr, o_ptr, 0L);
    autopick_alter_item(caster_ptr, item, (bool)(destroy_identify && !old_known));
    return TRUE;
}
