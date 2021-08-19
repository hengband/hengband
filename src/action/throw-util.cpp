/*!
 * @file throw-util.cpp
 * @brief 投擲処理関連クラス
 * @date 2021/08/20
 * @author Hourier
 */

#include "action/throw-util.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-info/equipment-info.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

it_type::it_type(player_type *creature_ptr, object_type *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken)
    : q_ptr(q_ptr)
    , mult(mult)
    , msec(delay_factor_val * delay_factor_val * delay_factor_val)
    , boomerang(boomerang)
    , shuriken(shuriken)
    , creature_ptr(creature_ptr)
{
}

bool it_type::check_can_throw()
{
    if (!this->check_what_throw())
        return false;

    if (object_is_cursed(this->o_ptr) && (this->item >= INVEN_MAIN_HAND)) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return false;
    }

    if (this->creature_ptr->current_floor_ptr->inside_arena && !this->boomerang && (this->o_ptr->tval != TV_SPIKE)) {
        msg_print(_("アリーナではアイテムを使えない！", "You're in the arena now. This is hand-to-hand!"));
        msg_print(NULL);
        return false;
    }

    return true;
}

bool it_type::check_what_throw()
{
    if (this->shuriken >= 0) {
        this->item = this->shuriken;
        this->o_ptr = &this->creature_ptr->inventory_list[this->item];
        return true;
    }

    concptr q, s;
    if (!check_throw_boomerang(&q, &s))
        return false;

    q = _("どのアイテムを投げますか? ", "Throw which item? ");
    s = _("投げるアイテムがない。", "You have nothing to throw.");
    this->o_ptr = choose_object(this->creature_ptr, &this->item, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP, TV_NONE);
    if (!this->o_ptr) {
        flush();
        return false;
    }

    return true;
}

bool it_type::check_throw_boomerang(concptr *q, concptr *s)
{
    if (!this->boomerang)
        return true;

    if (has_melee_weapon(this->creature_ptr, INVEN_MAIN_HAND) && has_melee_weapon(this->creature_ptr, INVEN_SUB_HAND)) {
        item_tester_hook = item_tester_hook_boomerang;
        *q = _("どの武器を投げますか? ", "Throw which item? ");
        *s = _("投げる武器がない。", "You have nothing to throw.");
        this->o_ptr = choose_object(this->creature_ptr, &this->item, *q, *s, USE_EQUIP, TV_NONE);
        if (!this->o_ptr) {
            flush();
            return false;
        }

        return true;
    }

    if (has_melee_weapon(this->creature_ptr, INVEN_SUB_HAND)) {
        this->item = INVEN_SUB_HAND;
        this->o_ptr = &this->creature_ptr->inventory_list[this->item];
        return true;
    }

    this->item = INVEN_MAIN_HAND;
    this->o_ptr = &this->creature_ptr->inventory_list[this->item];
    return true;
}
