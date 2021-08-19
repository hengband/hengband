/*!
 * @file throw-util.cpp
 * @brief 投擲処理関連クラス
 * @date 2021/08/20
 * @author Hourier
 */

#include "action/throw-util.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "core/stuff-handler.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "object/object-stack.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "specific-object/torch.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

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

void it_type::calc_throw_range()
{
    this->q_ptr->copy_from(this->o_ptr);
    object_flags(this->creature_ptr, this->q_ptr, this->obj_flags);
    torch_flags(this->q_ptr, this->obj_flags);
    distribute_charges(this->o_ptr, this->q_ptr, 1);
    this->q_ptr->number = 1;
    describe_flavor(this->creature_ptr, this->o_name, this->q_ptr, OD_OMIT_PREFIX);
    if (this->creature_ptr->mighty_throw)
        this->mult += 3;

    int mul = 10 + 2 * (this->mult - 1);
    int div = ((this->q_ptr->weight > 10) ? this->q_ptr->weight : 10);
    if ((has_flag(this->obj_flags, TR_THROW)) || this->boomerang)
        div /= 2;

    this->tdis = (adj_str_blow[this->creature_ptr->stat_index[A_STR]] + 20) * mul / div;
    if (this->tdis > mul)
        this->tdis = mul;
}

bool it_type::calc_throw_grid()
{
    if (this->shuriken >= 0) {
        this->ty = randint0(101) - 50 + this->creature_ptr->y;
        this->tx = randint0(101) - 50 + this->creature_ptr->x;
        return true;
    }

    project_length = this->tdis + 1;
    DIRECTION dir;
    if (!get_aim_dir(this->creature_ptr, &dir))
        return false;

    this->tx = this->creature_ptr->x + 99 * ddx[dir];
    this->ty = this->creature_ptr->y + 99 * ddy[dir];
    if ((dir == 5) && target_okay(this->creature_ptr)) {
        this->tx = target_col;
        this->ty = target_row;
    }

    project_length = 0;
    return true;
}

void it_type::reflect_inventory_by_throw()
{
    if ((this->q_ptr->name1 == ART_MJOLLNIR) || (this->q_ptr->name1 == ART_AEGISFANG) || this->boomerang)
        this->return_when_thrown = true;

    if (this->item < 0) {
        floor_item_increase(this->creature_ptr, 0 - this->item, -1);
        floor_item_optimize(this->creature_ptr, 0 - this->item);
        return;
    }

    inven_item_increase(this->creature_ptr, this->item, -1);
    if (!this->return_when_thrown)
        inven_item_describe(this->creature_ptr, this->item);

    inven_item_optimize(this->creature_ptr, this->item);
}

void it_type::set_class_specific_throw_params()
{
    PlayerEnergy energy(this->creature_ptr);
    energy.set_player_turn_energy(100);
    if ((this->creature_ptr->pclass == CLASS_ROGUE) || (this->creature_ptr->pclass == CLASS_NINJA)) {
        energy.sub_player_turn_energy(this->creature_ptr->lev);
    }

    this->y = this->creature_ptr->y;
    this->x = this->creature_ptr->x;
    handle_stuff(this->creature_ptr);
    this->shuriken = (this->creature_ptr->pclass == CLASS_NINJA)
        && ((this->q_ptr->tval == TV_SPIKE) || ((has_flag(this->obj_flags, TR_THROW)) && (this->q_ptr->tval == TV_SWORD)));
}

void it_type::set_racial_chance()
{
    if (has_flag(this->obj_flags, TR_THROW))
        this->chance = ((this->creature_ptr->skill_tht) + ((this->creature_ptr->to_h_b + this->q_ptr->to_h) * BTH_PLUS_ADJ));
    else
        this->chance = (this->creature_ptr->skill_tht + (this->creature_ptr->to_h_b * BTH_PLUS_ADJ));

    if (this->shuriken != 0)
        this->chance *= 2;
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
