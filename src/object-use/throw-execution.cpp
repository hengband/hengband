/*!
 * @file throw-util.cpp
 * @brief 投擲処理関連クラス
 * @date 2021/08/20
 * @author Hourier
 */

#include "object-use/throw-execution.h"
#include "action/weapon-shield.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "combat/slaying.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
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
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-weapon.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player/player-status-table.h"
#include "racial/racial-android.h"
#include "specific-object/torch.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"
#include "wizard/wizard-messages.h"

ObjectThrowEntity::ObjectThrowEntity(PlayerType *player_ptr, ObjectType *q_ptr, const int delay_factor_val, const int mult, const bool boomerang, const OBJECT_IDX shuriken)
    : q_ptr(q_ptr)
    , player_ptr(player_ptr)
    , shuriken(shuriken)
    , mult(mult)
    , msec(delay_factor_val)
    , boomerang(boomerang)
{
}

bool ObjectThrowEntity::check_can_throw()
{
    if (!this->check_what_throw()) {
        return false;
    }

    if (this->o_ptr->is_cursed() && (this->item >= INVEN_MAIN_HAND)) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return false;
    }

    if (this->player_ptr->current_floor_ptr->inside_arena && !this->boomerang && (this->o_ptr->tval != ItemKindType::SPIKE)) {
        msg_print(_("アリーナではアイテムを使えない！", "You're in the arena now. This is hand-to-hand!"));
        msg_print(nullptr);
        return false;
    }

    return true;
}

void ObjectThrowEntity::calc_throw_range()
{
    this->q_ptr->copy_from(this->o_ptr);
    this->obj_flags = object_flags(this->q_ptr);
    torch_flags(this->q_ptr, this->obj_flags);
    distribute_charges(this->o_ptr, this->q_ptr, 1);
    this->q_ptr->number = 1;
    describe_flavor(this->player_ptr, this->o_name, this->q_ptr, OD_OMIT_PREFIX);
    if (this->player_ptr->mighty_throw) {
        this->mult += 3;
    }

    auto mul = 10 + 2 * (this->mult - 1);
    auto div = ((this->q_ptr->weight > 10) ? this->q_ptr->weight : 10);
    if ((this->obj_flags.has(TR_THROW)) || this->boomerang) {
        div /= 2;
    }

    this->tdis = (adj_str_blow[this->player_ptr->stat_index[A_STR]] + 20) * mul / div;
    if (this->tdis > mul) {
        this->tdis = mul;
    }
}

bool ObjectThrowEntity::calc_throw_grid()
{
    if (this->shuriken >= 0) {
        this->ty = randint0(101) - 50 + this->player_ptr->y;
        this->tx = randint0(101) - 50 + this->player_ptr->x;
        return true;
    }

    project_length = this->tdis + 1;
    DIRECTION dir;
    if (!get_aim_dir(this->player_ptr, &dir)) {
        return false;
    }

    this->tx = this->player_ptr->x + 99 * ddx[dir];
    this->ty = this->player_ptr->y + 99 * ddy[dir];
    if ((dir == 5) && target_okay(this->player_ptr)) {
        this->tx = target_col;
        this->ty = target_row;
    }

    project_length = 0;
    return true;
}

void ObjectThrowEntity::reflect_inventory_by_throw()
{
    if ((this->q_ptr->fixed_artifact_idx == ART_MJOLLNIR) || (this->q_ptr->fixed_artifact_idx == ART_AEGISFANG) || this->boomerang) {
        this->return_when_thrown = true;
    }

    if (this->item < 0) {
        floor_item_increase(this->player_ptr, 0 - this->item, -1);
        floor_item_optimize(this->player_ptr, 0 - this->item);
        return;
    }

    inven_item_increase(this->player_ptr, this->item, -1);
    if (!this->return_when_thrown) {
        inven_item_describe(this->player_ptr, this->item);
    }

    inven_item_optimize(this->player_ptr, this->item);
}

void ObjectThrowEntity::set_class_specific_throw_params()
{
    PlayerEnergy energy(this->player_ptr);
    energy.set_player_turn_energy(100);
    PlayerClass pc(this->player_ptr);
    if (pc.equals(PlayerClassType::ROGUE) || pc.equals(PlayerClassType::NINJA)) {
        energy.sub_player_turn_energy(this->player_ptr->lev);
    }

    this->y = this->player_ptr->y;
    this->x = this->player_ptr->x;
    handle_stuff(this->player_ptr);
    this->shuriken = pc.equals(PlayerClassType::NINJA) && ((this->q_ptr->tval == ItemKindType::SPIKE) || ((this->obj_flags.has(TR_THROW)) && (this->q_ptr->tval == ItemKindType::SWORD)));
}

void ObjectThrowEntity::set_racial_chance()
{
    auto compensation = this->obj_flags.has(TR_THROW) ? this->q_ptr->to_h : 0;
    this->chance = this->player_ptr->skill_tht + (this->player_ptr->to_h_b + compensation) * BTH_PLUS_ADJ;
    if (this->shuriken != 0) {
        this->chance *= 2;
    }
}

void ObjectThrowEntity::exe_throw()
{
    this->cur_dis = 0;
    while (this->cur_dis <= this->tdis) {
        if ((this->y == this->ty) && (this->x == this->tx)) {
            break;
        }

        if (this->check_racial_target_bold()) {
            break;
        }

        this->check_racial_target_seen();
        if (this->check_racial_target_monster()) {
            continue;
        }

        auto *floor_ptr = this->player_ptr->current_floor_ptr;
        this->g_ptr = &floor_ptr->grid_array[this->y][this->x];
        this->m_ptr = &floor_ptr->m_list[this->g_ptr->m_idx];
        monster_name(this->player_ptr, this->g_ptr->m_idx, this->m_name);
        this->visible = this->m_ptr->ml;
        this->hit_body = true;
        this->attack_racial_power();
        break;
    }
}

void ObjectThrowEntity::display_figurine_throw()
{
    if ((this->q_ptr->tval != ItemKindType::FIGURINE) || this->player_ptr->current_floor_ptr->inside_arena) {
        return;
    }

    this->corruption_possibility = 100;
    auto figure_r_idx = i2enum<MonsterRaceId>(this->q_ptr->pval);
    if (!(summon_named_creature(this->player_ptr, 0, this->y, this->x, figure_r_idx, !(this->q_ptr->is_cursed()) ? PM_FORCE_PET : PM_NONE))) {
        msg_print(_("人形は捻じ曲がり砕け散ってしまった！", "The Figurine writhes and then shatters."));
        return;
    }

    if (this->q_ptr->is_cursed()) {
        msg_print(_("これはあまり良くない気がする。", "You have a bad feeling about this."));
    }
}

void ObjectThrowEntity::display_potion_throw()
{
    if (!this->q_ptr->is_potion()) {
        return;
    }

    if (!this->hit_body && !this->hit_wall && (randint1(100) >= this->corruption_possibility)) {
        this->corruption_possibility = 0;
        return;
    }

    msg_format(_("%sは砕け散った！", "The %s shatters!"), this->o_name);
    if (!potion_smash_effect(this->player_ptr, 0, this->y, this->x, this->q_ptr->k_idx)) {
        this->do_drop = false;
        return;
    }

    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    auto *angry_m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[this->y][this->x].m_idx];
    if ((floor_ptr->grid_array[this->y][this->x].m_idx == 0) || !is_friendly(angry_m_ptr) || monster_invulner_remaining(angry_m_ptr)) {
        this->do_drop = false;
        return;
    }

    GAME_TEXT angry_m_name[MAX_NLEN];
    monster_desc(this->player_ptr, angry_m_name, angry_m_ptr, 0);
    msg_format(_("%sは怒った！", "%^s gets angry!"), angry_m_name);
    set_hostile(this->player_ptr, &floor_ptr->m_list[floor_ptr->grid_array[this->y][this->x].m_idx]);
    this->do_drop = false;
}

void ObjectThrowEntity::check_boomerang_throw()
{
    if (!this->return_when_thrown) {
        return;
    }

    this->back_chance = randint1(30) + 20 + ((int)(adj_dex_th[this->player_ptr->stat_index[A_DEX]]) - 128);
    this->super_boomerang = (((this->q_ptr->fixed_artifact_idx == ART_MJOLLNIR) || (this->q_ptr->fixed_artifact_idx == ART_AEGISFANG)) && this->boomerang);
    this->corruption_possibility = -1;
    if (this->boomerang) {
        this->back_chance += 4 + randint1(5);
    }

    if (this->super_boomerang) {
        this->back_chance += 100;
    }

    describe_flavor(this->player_ptr, this->o2_name, this->q_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    this->process_boomerang_throw();
}

void ObjectThrowEntity::process_boomerang_back()
{
    if (this->come_back) {
        if ((this->item != INVEN_MAIN_HAND) && (this->item != INVEN_SUB_HAND)) {
            store_item_to_inventory(player_ptr, this->q_ptr);
            this->do_drop = false;
            return;
        }

        this->o_ptr = &player_ptr->inventory_list[this->item];
        this->o_ptr->copy_from(this->q_ptr);
        player_ptr->equip_cnt++;
        player_ptr->update |= PU_BONUS | PU_TORCH | PU_MANA;
        player_ptr->window_flags |= PW_EQUIP;
        this->do_drop = false;
        return;
    }

    if (this->equiped_item) {
        verify_equip_slot(player_ptr, this->item);
        calc_android_exp(player_ptr);
    }
}

void ObjectThrowEntity::drop_thrown_item()
{
    if (!this->do_drop) {
        return;
    }

    auto is_bold = cave_has_flag_bold(this->player_ptr->current_floor_ptr, this->y, this->x, FloorFeatureType::PROJECT);
    auto drop_y = is_bold ? this->y : this->prev_y;
    auto drop_x = is_bold ? this->x : this->prev_x;
    (void)drop_near(this->player_ptr, this->q_ptr, this->corruption_possibility, drop_y, drop_x);
}

bool ObjectThrowEntity::check_what_throw()
{
    if (this->shuriken >= 0) {
        this->item = this->shuriken;
        this->o_ptr = &this->player_ptr->inventory_list[this->item];
        return true;
    }

    concptr q, s;
    if (!this->check_throw_boomerang(&q, &s)) {
        return false;
    }

    q = _("どのアイテムを投げますか? ", "Throw which item? ");
    s = _("投げるアイテムがない。", "You have nothing to throw.");
    this->o_ptr = choose_object(this->player_ptr, &this->item, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP);
    if (!this->o_ptr) {
        flush();
        return false;
    }

    return true;
}

bool ObjectThrowEntity::check_throw_boomerang(concptr *q, concptr *s)
{
    if (!this->boomerang) {
        return true;
    }

    if (has_melee_weapon(this->player_ptr, INVEN_MAIN_HAND) && has_melee_weapon(this->player_ptr, INVEN_SUB_HAND)) {
        *q = _("どの武器を投げますか? ", "Throw which item? ");
        *s = _("投げる武器がない。", "You have nothing to throw.");
        this->o_ptr = choose_object(this->player_ptr, &this->item, *q, *s, USE_EQUIP, FuncItemTester(&ObjectType::is_throwable));
        if (!this->o_ptr) {
            flush();
            return false;
        }

        return true;
    }

    if (has_melee_weapon(this->player_ptr, INVEN_SUB_HAND)) {
        this->item = INVEN_SUB_HAND;
        this->o_ptr = &this->player_ptr->inventory_list[this->item];
        return true;
    }

    this->item = INVEN_MAIN_HAND;
    this->o_ptr = &this->player_ptr->inventory_list[this->item];
    return true;
}

bool ObjectThrowEntity::check_racial_target_bold()
{
    this->ny[this->cur_dis] = this->y;
    this->nx[this->cur_dis] = this->x;
    mmove2(&this->ny[this->cur_dis], &this->nx[this->cur_dis], this->player_ptr->y, this->player_ptr->x, this->ty, this->tx);
    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    if (cave_has_flag_bold(floor_ptr, this->ny[this->cur_dis], this->nx[this->cur_dis], FloorFeatureType::PROJECT)) {
        return false;
    }

    this->hit_wall = true;
    return (this->q_ptr->tval == ItemKindType::FIGURINE) || this->q_ptr->is_potion() || (floor_ptr->grid_array[this->ny[this->cur_dis]][this->nx[this->cur_dis]].m_idx == 0);
}

void ObjectThrowEntity::check_racial_target_seen()
{
    if (!panel_contains(this->ny[this->cur_dis], this->nx[this->cur_dis]) || !player_can_see_bold(this->player_ptr, this->ny[this->cur_dis], this->nx[this->cur_dis])) {
        term_xtra(TERM_XTRA_DELAY, this->msec);
        return;
    }

    if (this->msec <= 0) {
        return;
    }

    auto c = object_char(this->q_ptr);
    auto a = object_attr(this->q_ptr);
    print_rel(this->player_ptr, c, a, this->ny[this->cur_dis], this->nx[this->cur_dis]);
    move_cursor_relative(this->ny[this->cur_dis], this->nx[this->cur_dis]);
    term_fresh();
    term_xtra(TERM_XTRA_DELAY, this->msec);
    lite_spot(this->player_ptr, this->ny[this->cur_dis], this->nx[this->cur_dis]);
    term_fresh();
}

bool ObjectThrowEntity::check_racial_target_monster()
{
    this->prev_y = this->y;
    this->prev_x = this->x;
    this->x = this->nx[this->cur_dis];
    this->y = this->ny[this->cur_dis];
    this->cur_dis++;
    return this->player_ptr->current_floor_ptr->grid_array[this->y][this->x].m_idx == 0;
}

void ObjectThrowEntity::attack_racial_power()
{
    if (!test_hit_fire(this->player_ptr, this->chance - this->cur_dis, this->m_ptr, this->m_ptr->ml, this->o_name)) {
        return;
    }

    this->display_attack_racial_power();
    this->calc_racial_power_damage();
    msg_format_wizard(this->player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), this->tdam,
        this->m_ptr->hp - this->tdam, this->m_ptr->maxhp, this->m_ptr->max_maxhp);

    auto fear = false;
    AttributeFlags attribute_flags{};
    attribute_flags.set(AttributeType::PLAYER_SHOOT);
    if (is_active_torch(this->o_ptr)) {
        attribute_flags.set(AttributeType::FIRE);
    }

    MonsterDamageProcessor mdp(this->player_ptr, this->g_ptr->m_idx, this->tdam, &fear, attribute_flags);
    if (mdp.mon_take_hit(extract_note_dies(real_r_idx(this->m_ptr)))) {
        return;
    }

    message_pain(this->player_ptr, this->g_ptr->m_idx, this->tdam);
    if ((this->tdam > 0) && !this->q_ptr->is_potion()) {
        anger_monster(this->player_ptr, this->m_ptr);
    }

    if (fear && this->m_ptr->ml) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), this->m_name);
    }
}

void ObjectThrowEntity::display_attack_racial_power()
{
    if (!this->visible) {
        msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), this->o_name);
        return;
    }

    msg_format(_("%sが%sに命中した。", "The %s hits %s."), this->o_name, this->m_name);
    if (!this->m_ptr->ml) {
        return;
    }

    if (!this->player_ptr->effects()->hallucination()->is_hallucinated()) {
        monster_race_track(this->player_ptr, this->m_ptr->ap_r_idx);
    }

    health_track(this->player_ptr, this->g_ptr->m_idx);
}

void ObjectThrowEntity::calc_racial_power_damage()
{
    auto dd = this->q_ptr->dd;
    auto ds = this->q_ptr->ds;
    torch_dice(this->q_ptr, &dd, &ds);
    this->tdam = damroll(dd, ds);
    this->tdam = calc_attack_damage_with_slay(this->player_ptr, this->q_ptr, this->tdam, this->m_ptr, HISSATSU_NONE, true);
    this->tdam = critical_shot(this->player_ptr, this->q_ptr->weight, this->q_ptr->to_h, 0, this->tdam);
    this->tdam += (this->q_ptr->to_d > 0 ? 1 : -1) * this->q_ptr->to_d;
    if (this->boomerang) {
        this->tdam *= (this->mult + this->player_ptr->num_blow[this->item - INVEN_MAIN_HAND]);
        this->tdam += this->player_ptr->to_d_m;
    } else if (this->obj_flags.has(TR_THROW)) {
        this->tdam *= (3 + this->mult);
        this->tdam += this->player_ptr->to_d_m;
    } else {
        this->tdam *= this->mult;
    }

    if (this->shuriken != 0) {
        this->tdam += ((this->player_ptr->lev + 30) * (this->player_ptr->lev + 30) - 900) / 55;
    }

    if (this->tdam < 0) {
        this->tdam = 0;
    }

    this->tdam = mon_damage_mod(this->player_ptr, this->m_ptr, this->tdam, false);
}

void ObjectThrowEntity::process_boomerang_throw()
{
    if ((this->back_chance <= 30) || (one_in_(100) && !this->super_boomerang)) {
        msg_format(_("%sが返ってこなかった！", "%s doesn't come back!"), this->o2_name);
        return;
    }

    for (auto i = this->cur_dis - 1; i > 0; i--) {
        if (!panel_contains(this->ny[i], this->nx[i]) || !player_can_see_bold(this->player_ptr, this->ny[i], this->nx[i])) {
            term_xtra(TERM_XTRA_DELAY, this->msec);
            continue;
        }

        auto c = object_char(this->q_ptr);
        auto a = object_attr(this->q_ptr);
        if (this->msec <= 0) {
            continue;
        }

        print_rel(this->player_ptr, c, a, this->ny[i], this->nx[i]);
        move_cursor_relative(this->ny[i], this->nx[i]);
        term_fresh();
        term_xtra(TERM_XTRA_DELAY, this->msec);
        lite_spot(this->player_ptr, this->ny[i], this->nx[i]);
        term_fresh();
    }

    this->display_boomerang_throw();
}

void ObjectThrowEntity::display_boomerang_throw()
{
    const auto is_blind = this->player_ptr->effects()->blindness()->is_blind();
    if ((this->back_chance > 37) && !is_blind && (this->item >= 0)) {
        msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), this->o2_name);
        this->come_back = true;
        return;
    }

    auto back_message = this->item >= 0 ? _("%sを受け損ねた！", "%s comes back, but you can't catch!") : _("%sが返ってきた。", "%s comes back.");
    msg_format(back_message, this->o2_name);
    this->y = this->player_ptr->y;
    this->x = this->player_ptr->x;
}
