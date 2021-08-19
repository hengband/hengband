/*!
 * @file throw-util.cpp
 * @brief 投擲処理関連クラス
 * @date 2021/08/20
 * @author Hourier
 */

#include "action/throw-util.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "combat/slaying.h"
#include "core/stuff-handler.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/cheat-types.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "specific-object/torch.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"
#include "wizard/wizard-messages.h"

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

void it_type::exe_throw()
{
    this->cur_dis = 0;
    while (this->cur_dis <= this->tdis) {
        if ((this->y == this->ty) && (this->x == this->tx))
            break;

        if (this->check_racial_target_bold())
            break;

        this->check_racial_target_seen();
        if (this->check_racial_target_monster())
            continue;

        this->g_ptr = &this->creature_ptr->current_floor_ptr->grid_array[this->y][this->x];
        this->m_ptr = &this->creature_ptr->current_floor_ptr->m_list[this->g_ptr->m_idx];
        monster_name(this->creature_ptr, this->g_ptr->m_idx, this->m_name);
        this->visible = this->m_ptr->ml;
        this->hit_body = true;
        this->attack_racial_power();
        break;
    }
}

void it_type::display_figurine_throw()
{
    if ((this->q_ptr->tval != TV_FIGURINE) || this->creature_ptr->current_floor_ptr->inside_arena)
        return;

    this->corruption_possibility = 100;
    if (!(summon_named_creature(this->creature_ptr, 0, this->y, this->x, this->q_ptr->pval, !(object_is_cursed(this->q_ptr)) ? PM_FORCE_PET : PM_NONE))) {
        msg_print(_("人形は捻じ曲がり砕け散ってしまった！", "The Figurine writhes and then shatters."));
        return;
    }

    if (object_is_cursed(this->q_ptr))
        msg_print(_("これはあまり良くない気がする。", "You have a bad feeling about this."));
}

void it_type::display_potion_throw()
{
    if (!object_is_potion(this->q_ptr))
        return;

    if (!this->hit_body && !this->hit_wall && (randint1(100) >= this->corruption_possibility)) {
        this->corruption_possibility = 0;
        return;
    }

    msg_format(_("%sは砕け散った！", "The %s shatters!"), this->o_name);
    if (!potion_smash_effect(this->creature_ptr, 0, this->y, this->x, this->q_ptr->k_idx)) {
        this->do_drop = false;
        return;
    }

    monster_type *angry_m_ptr = &this->creature_ptr->current_floor_ptr->m_list[this->creature_ptr->current_floor_ptr->grid_array[this->y][this->x].m_idx];
    if ((this->creature_ptr->current_floor_ptr->grid_array[this->y][this->x].m_idx == 0) || !is_friendly(angry_m_ptr) || monster_invulner_remaining(angry_m_ptr)) {
        this->do_drop = false;
        return;
    }

    GAME_TEXT angry_m_name[MAX_NLEN];
    monster_desc(this->creature_ptr, angry_m_name, angry_m_ptr, 0);
    msg_format(_("%sは怒った！", "%^s gets angry!"), angry_m_name);
    set_hostile(this->creature_ptr, &this->creature_ptr->current_floor_ptr->m_list[this->creature_ptr->current_floor_ptr->grid_array[this->y][this->x].m_idx]);
    this->do_drop = false;
}

bool it_type::check_what_throw()
{
    if (this->shuriken >= 0) {
        this->item = this->shuriken;
        this->o_ptr = &this->creature_ptr->inventory_list[this->item];
        return true;
    }

    concptr q, s;
    if (!this->check_throw_boomerang(&q, &s))
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

bool it_type::check_racial_target_bold()
{
    this->ny[this->cur_dis] = this->y;
    this->nx[this->cur_dis] = this->x;
    mmove2(&this->ny[this->cur_dis], &this->nx[this->cur_dis], this->creature_ptr->y, this->creature_ptr->x, this->ty, this->tx);
    if (cave_has_flag_bold(this->creature_ptr->current_floor_ptr, this->ny[this->cur_dis], this->nx[this->cur_dis], FF_PROJECT))
        return false;

    this->hit_wall = true;
    return (this->q_ptr->tval == TV_FIGURINE) || object_is_potion(this->q_ptr)
        || (this->creature_ptr->current_floor_ptr->grid_array[this->ny[this->cur_dis]][this->nx[this->cur_dis]].m_idx == 0);
}

void it_type::check_racial_target_seen()
{
    if (!panel_contains(this->ny[this->cur_dis], this->nx[this->cur_dis])
        || !player_can_see_bold(this->creature_ptr, this->ny[this->cur_dis], this->nx[this->cur_dis])) {
        term_xtra(TERM_XTRA_DELAY, this->msec);
        return;
    }

    if (this->msec > 0) {
        SYMBOL_CODE c = object_char(this->q_ptr);
        TERM_COLOR a = object_attr(this->q_ptr);
        print_rel(this->creature_ptr, c, a, this->ny[this->cur_dis], this->nx[this->cur_dis]);
        move_cursor_relative(this->ny[this->cur_dis], this->nx[this->cur_dis]);
        term_fresh();
        term_xtra(TERM_XTRA_DELAY, this->msec);
        lite_spot(this->creature_ptr, this->ny[this->cur_dis], this->nx[this->cur_dis]);
        term_fresh();
    }
}

bool it_type::check_racial_target_monster()
{
    this->prev_y = this->y;
    this->prev_x = this->x;
    this->x = this->nx[this->cur_dis];
    this->y = this->ny[this->cur_dis];
    this->cur_dis++;
    return this->creature_ptr->current_floor_ptr->grid_array[this->y][this->x].m_idx == 0;
}

void it_type::attack_racial_power()
{
    if (!test_hit_fire(this->creature_ptr, this->chance - this->cur_dis, this->m_ptr, this->m_ptr->ml, this->o_name))
        return;

    this->display_attack_racial_power();
    this->calc_racial_power_damage();
    msg_format_wizard(this->creature_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), this->tdam,
        this->m_ptr->hp - this->tdam, this->m_ptr->maxhp, this->m_ptr->max_maxhp);

    bool fear = false;
    MonsterDamageProcessor mdp(this->creature_ptr, this->g_ptr->m_idx, this->tdam, &fear);
    if (mdp.mon_take_hit(extract_note_dies(real_r_idx(this->m_ptr))))
        return;

    message_pain(this->creature_ptr, this->g_ptr->m_idx, this->tdam);
    if ((this->tdam > 0) && !object_is_potion(this->q_ptr))
        anger_monster(this->creature_ptr, this->m_ptr);

    if (fear && this->m_ptr->ml) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), this->m_name);
    }
}

void it_type::display_attack_racial_power()
{
    if (!this->visible) {
        msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), this->o_name);
        return;
    }

    msg_format(_("%sが%sに命中した。", "The %s hits %s."), this->o_name, this->m_name);
    if (!this->m_ptr->ml)
        return;

    if (!this->creature_ptr->image)
        monster_race_track(this->creature_ptr, this->m_ptr->ap_r_idx);

    health_track(this->creature_ptr, this->g_ptr->m_idx);
}

void it_type::calc_racial_power_damage()
{
    int dd = this->q_ptr->dd;
    int ds = this->q_ptr->ds;
    torch_dice(this->q_ptr, &dd, &ds);
    this->tdam = damroll(dd, ds);
    this->tdam = calc_attack_damage_with_slay(this->creature_ptr, this->q_ptr, this->tdam, this->m_ptr, HISSATSU_NONE, true);
    this->tdam = critical_shot(this->creature_ptr, this->q_ptr->weight, this->q_ptr->to_h, 0, this->tdam);
    if (this->q_ptr->to_d > 0)
        this->tdam += this->q_ptr->to_d;
    else
        this->tdam += -this->q_ptr->to_d;

    if (this->boomerang) {
        this->tdam *= (this->mult + this->creature_ptr->num_blow[this->item - INVEN_MAIN_HAND]);
        this->tdam += this->creature_ptr->to_d_m;
    } else if (has_flag(this->obj_flags, TR_THROW)) {
        this->tdam *= (3 + this->mult);
        this->tdam += this->creature_ptr->to_d_m;
    } else {
        this->tdam *= this->mult;
    }

    if (this->shuriken != 0)
        this->tdam += ((this->creature_ptr->lev + 30) * (this->creature_ptr->lev + 30) - 900) / 55;

    if (this->tdam < 0)
        this->tdam = 0;

    this->tdam = mon_damage_mod(this->creature_ptr, this->m_ptr, this->tdam, false);
}
