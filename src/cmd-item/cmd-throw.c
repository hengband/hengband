#include "cmd-item/cmd-throw.h"
#include "action/throw-util.h"
#include "action/weapon-shield.h"
#include "artifact/fixed-art-types.h"
#include "combat/attack-power-table.h"
#include "combat/shoot.h"
#include "combat/slaying.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/cheat-types.h"
#include "game-option/special-options.h"
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
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "player/attack-defense-types.h"
#include "player/player-status-table.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "specific-object/torch.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"
#include "wizard/wizard-messages.h"

static bool check_throw_boomerang(player_type *creature_ptr, it_type *it_ptr, concptr *q, concptr *s)
{
    if (!it_ptr->boomerang)
        return TRUE;

    if (has_melee_weapon(creature_ptr, INVEN_RARM) && has_melee_weapon(creature_ptr, INVEN_LARM)) {
        item_tester_hook = item_tester_hook_boomerang;
        *q = _("どの武器を投げますか? ", "Throw which it_ptr->item? ");
        *s = _("投げる武器がない。", "You have nothing to throw.");
        it_ptr->o_ptr = choose_object(creature_ptr, &it_ptr->item, *q, *s, USE_EQUIP, 0);
        if (!it_ptr->o_ptr) {
            flush();
            return FALSE;
        }

        return TRUE;
    }

    if (has_melee_weapon(creature_ptr, INVEN_LARM)) {
        it_ptr->item = INVEN_LARM;
        it_ptr->o_ptr = &creature_ptr->inventory_list[it_ptr->item];
        return TRUE;
    }

    it_ptr->item = INVEN_RARM;
    it_ptr->o_ptr = &creature_ptr->inventory_list[it_ptr->item];
    return TRUE;
}

static bool check_what_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if (it_ptr->shuriken >= 0) {
        it_ptr->item = it_ptr->shuriken;
        it_ptr->o_ptr = &creature_ptr->inventory_list[it_ptr->item];
        return TRUE;
    }

    concptr q, s;
    if (!check_throw_boomerang(creature_ptr, it_ptr, &q, &s))
        return FALSE;

    q = _("どのアイテムを投げますか? ", "Throw which it_ptr->item? ");
    s = _("投げるアイテムがない。", "You have nothing to throw.");
    it_ptr->o_ptr = choose_object(creature_ptr, &it_ptr->item, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP, 0);
    if (!it_ptr->o_ptr) {
        flush();
        return FALSE;
    }

    return TRUE;
}

static bool check_can_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if (!check_what_throw(creature_ptr, it_ptr))
        return FALSE;

    if (object_is_cursed(it_ptr->o_ptr) && (it_ptr->item >= INVEN_RARM)) {
        msg_print(_("ふーむ、どうやら呪われているようだ。", "Hmmm, it seems to be cursed."));
        return FALSE;
    }

    if (creature_ptr->current_floor_ptr->inside_arena && !it_ptr->boomerang && (it_ptr->o_ptr->tval != TV_SPIKE)) {
        msg_print(_("アリーナではアイテムを使えない！", "You're in the arena now. This is hand-to-hand!"));
        msg_print(NULL);
        return FALSE;
    }

    return TRUE;
}

static void calc_throw_range(player_type *creature_ptr, it_type *it_ptr)
{
    object_copy(it_ptr->q_ptr, it_ptr->o_ptr);
    object_flags(creature_ptr, it_ptr->q_ptr, it_ptr->obj_flags);
    torch_flags(it_ptr->q_ptr, it_ptr->obj_flags);
    distribute_charges(it_ptr->o_ptr, it_ptr->q_ptr, 1);
    it_ptr->q_ptr->number = 1;
    describe_flavor(creature_ptr, it_ptr->o_name, it_ptr->q_ptr, OD_OMIT_PREFIX);
    if (creature_ptr->mighty_throw)
        it_ptr->mult += 3;

    int mul = 10 + 2 * (it_ptr->mult - 1);
    int div = ((it_ptr->q_ptr->weight > 10) ? it_ptr->q_ptr->weight : 10);
    if ((has_flag(it_ptr->obj_flags, TR_THROW)) || it_ptr->boomerang)
        div /= 2;

    it_ptr->tdis = (adj_str_blow[creature_ptr->stat_ind[A_STR]] + 20) * mul / div;
    if (it_ptr->tdis > mul)
        it_ptr->tdis = mul;
}

static bool calc_throw_grid(player_type *creature_ptr, it_type *it_ptr)
{
    if (it_ptr->shuriken >= 0) {
        it_ptr->ty = randint0(101) - 50 + creature_ptr->y;
        it_ptr->tx = randint0(101) - 50 + creature_ptr->x;
        return TRUE;
    }

    project_length = it_ptr->tdis + 1;
    DIRECTION dir;
    if (!get_aim_dir(creature_ptr, &dir))
        return FALSE;

    it_ptr->tx = creature_ptr->x + 99 * ddx[dir];
    it_ptr->ty = creature_ptr->y + 99 * ddy[dir];
    if ((dir == 5) && target_okay(creature_ptr)) {
        it_ptr->tx = target_col;
        it_ptr->ty = target_row;
    }

    project_length = 0;
    return TRUE;
}

static void reflect_inventory_by_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if ((it_ptr->q_ptr->name1 == ART_MJOLLNIR) || (it_ptr->q_ptr->name1 == ART_AEGISFANG) || it_ptr->boomerang)
        it_ptr->return_when_thrown = TRUE;

    if (it_ptr->item < 0) {
        floor_item_increase(creature_ptr->current_floor_ptr, 0 - it_ptr->item, -1);
        floor_item_optimize(creature_ptr, 0 - it_ptr->item);
        return;
    }

    inven_item_increase(creature_ptr, it_ptr->item, -1);
    if (!it_ptr->return_when_thrown)
        inven_item_describe(creature_ptr, it_ptr->item);

    inven_item_optimize(creature_ptr, it_ptr->item);
}

static void set_class_specific_throw_params(player_type *creature_ptr, it_type *it_ptr)
{
    take_turn(creature_ptr, 100);
    if ((creature_ptr->pclass == CLASS_ROGUE) || (creature_ptr->pclass == CLASS_NINJA))
        creature_ptr->energy_use -= creature_ptr->lev;

    it_ptr->y = creature_ptr->y;
    it_ptr->x = creature_ptr->x;
    handle_stuff(creature_ptr);
    it_ptr->shuriken = (creature_ptr->pclass == CLASS_NINJA)
        && ((it_ptr->q_ptr->tval == TV_SPIKE) || ((has_flag(it_ptr->obj_flags, TR_THROW)) && (it_ptr->q_ptr->tval == TV_SWORD)));
}

static void set_racial_chance(player_type *creature_ptr, it_type *it_ptr)
{
    if (has_flag(it_ptr->obj_flags, TR_THROW))
        it_ptr->chance = ((creature_ptr->skill_tht) + ((creature_ptr->to_h_b + it_ptr->q_ptr->to_h) * BTH_PLUS_ADJ));
    else
        it_ptr->chance = (creature_ptr->skill_tht + (creature_ptr->to_h_b * BTH_PLUS_ADJ));

    if (it_ptr->shuriken != 0)
        it_ptr->chance *= 2;
}

static bool check_racial_target_bold(player_type *creature_ptr, it_type *it_ptr)
{
    it_ptr->ny[it_ptr->cur_dis] = it_ptr->y;
    it_ptr->nx[it_ptr->cur_dis] = it_ptr->x;
    mmove2(&it_ptr->ny[it_ptr->cur_dis], &it_ptr->nx[it_ptr->cur_dis], creature_ptr->y, creature_ptr->x, it_ptr->ty, it_ptr->tx);
    if (cave_has_flag_bold(creature_ptr->current_floor_ptr, it_ptr->ny[it_ptr->cur_dis], it_ptr->nx[it_ptr->cur_dis], FF_PROJECT))
        return FALSE;

    it_ptr->hit_wall = TRUE;
    return (it_ptr->q_ptr->tval == TV_FIGURINE) || object_is_potion(it_ptr->q_ptr)
        || (creature_ptr->current_floor_ptr->grid_array[it_ptr->ny[it_ptr->cur_dis]][it_ptr->nx[it_ptr->cur_dis]].m_idx == 0);
}

static void check_racial_target_seen(player_type *creature_ptr, it_type *it_ptr)
{
    if (!panel_contains(it_ptr->ny[it_ptr->cur_dis], it_ptr->nx[it_ptr->cur_dis])
        || !player_can_see_bold(creature_ptr, it_ptr->ny[it_ptr->cur_dis], it_ptr->nx[it_ptr->cur_dis])) {
        term_xtra(TERM_XTRA_DELAY, it_ptr->msec);
        return;
    }

    SYMBOL_CODE c = object_char(it_ptr->q_ptr);
    TERM_COLOR a = object_attr(it_ptr->q_ptr);
    print_rel(creature_ptr, c, a, it_ptr->ny[it_ptr->cur_dis], it_ptr->nx[it_ptr->cur_dis]);
    move_cursor_relative(it_ptr->ny[it_ptr->cur_dis], it_ptr->nx[it_ptr->cur_dis]);
    term_fresh();
    term_xtra(TERM_XTRA_DELAY, it_ptr->msec);
    lite_spot(creature_ptr, it_ptr->ny[it_ptr->cur_dis], it_ptr->nx[it_ptr->cur_dis]);
    term_fresh();
}

static bool check_racial_target_monster(player_type *creature_ptr, it_type *it_ptr)
{
    it_ptr->prev_y = it_ptr->y;
    it_ptr->prev_x = it_ptr->x;
    it_ptr->x = it_ptr->nx[it_ptr->cur_dis];
    it_ptr->y = it_ptr->ny[it_ptr->cur_dis];
    it_ptr->cur_dis++;
    return creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x].m_idx == 0;
}

static void display_attack_racial_power(player_type *creature_ptr, it_type *it_ptr)
{
    if (!it_ptr->visible) {
        msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), it_ptr->o_name);
        return;
    }

    msg_format(_("%sが%sに命中した。", "The %s hits %s."), it_ptr->o_name, it_ptr->m_name);
    if (!it_ptr->m_ptr->ml)
        return;

    if (!creature_ptr->image)
        monster_race_track(creature_ptr, it_ptr->m_ptr->ap_r_idx);

    health_track(creature_ptr, it_ptr->g_ptr->m_idx);
}

static void calc_racial_power_damage(player_type *creature_ptr, it_type *it_ptr)
{
    int dd = it_ptr->q_ptr->dd;
    int ds = it_ptr->q_ptr->ds;
    torch_dice(it_ptr->q_ptr, &dd, &ds);
    it_ptr->tdam = damroll(dd, ds);
    it_ptr->tdam = calc_attack_damage_with_slay(creature_ptr, it_ptr->q_ptr, it_ptr->tdam, it_ptr->m_ptr, 0, TRUE);
    it_ptr->tdam = critical_shot(creature_ptr, it_ptr->q_ptr->weight, it_ptr->q_ptr->to_h, 0, it_ptr->tdam);
    if (it_ptr->q_ptr->to_d > 0)
        it_ptr->tdam += it_ptr->q_ptr->to_d;
    else
        it_ptr->tdam += -it_ptr->q_ptr->to_d;

    if (it_ptr->boomerang) {
        it_ptr->tdam *= (it_ptr->mult + creature_ptr->num_blow[it_ptr->item - INVEN_RARM]);
        it_ptr->tdam += creature_ptr->to_d_m;
    } else if (has_flag(it_ptr->obj_flags, TR_THROW)) {
        it_ptr->tdam *= (3 + it_ptr->mult);
        it_ptr->tdam += creature_ptr->to_d_m;
    } else {
        it_ptr->tdam *= it_ptr->mult;
    }

    if (it_ptr->shuriken != 0)
        it_ptr->tdam += ((creature_ptr->lev + 30) * (creature_ptr->lev + 30) - 900) / 55;

    if (it_ptr->tdam < 0)
        it_ptr->tdam = 0;

    it_ptr->tdam = mon_damage_mod(creature_ptr, it_ptr->m_ptr, it_ptr->tdam, FALSE);
}

static void attack_racial_power(player_type *creature_ptr, it_type *it_ptr)
{
    if (!test_hit_fire(creature_ptr, it_ptr->chance - it_ptr->cur_dis, it_ptr->m_ptr, it_ptr->m_ptr->ml, it_ptr->o_name))
        return;

    display_attack_racial_power(creature_ptr, it_ptr);
    calc_racial_power_damage(creature_ptr, it_ptr);
    msg_format_wizard(creature_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), it_ptr->tdam,
        it_ptr->m_ptr->hp - it_ptr->tdam, it_ptr->m_ptr->maxhp, it_ptr->m_ptr->max_maxhp);

    bool fear = FALSE;
    if (mon_take_hit(creature_ptr, it_ptr->g_ptr->m_idx, it_ptr->tdam, &fear, extract_note_dies(real_r_idx(it_ptr->m_ptr))))
        return;

    message_pain(creature_ptr, it_ptr->g_ptr->m_idx, it_ptr->tdam);
    if ((it_ptr->tdam > 0) && !object_is_potion(it_ptr->q_ptr))
        anger_monster(creature_ptr, it_ptr->m_ptr);

    if (fear && it_ptr->m_ptr->ml) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), it_ptr->m_name);
    }
}

static void exe_throw(player_type *creature_ptr, it_type *it_ptr)
{
    it_ptr->cur_dis = 0;
    while (it_ptr->cur_dis <= it_ptr->tdis) {
        if ((it_ptr->y == it_ptr->ty) && (it_ptr->x == it_ptr->tx))
            break;

        if (check_racial_target_bold(creature_ptr, it_ptr))
            break;

        check_racial_target_seen(creature_ptr, it_ptr);
        if (check_racial_target_monster(creature_ptr, it_ptr))
            continue;

        it_ptr->g_ptr = &creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x];
        it_ptr->m_ptr = &creature_ptr->current_floor_ptr->m_list[it_ptr->g_ptr->m_idx];
        monster_name(creature_ptr, it_ptr->g_ptr->m_idx, it_ptr->m_name);
        it_ptr->visible = it_ptr->m_ptr->ml;
        it_ptr->hit_body = TRUE;
        attack_racial_power(creature_ptr, it_ptr);
        break;
    }
}

void display_figurine_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if ((it_ptr->q_ptr->tval != TV_FIGURINE) || creature_ptr->current_floor_ptr->inside_arena)
        return;

    it_ptr->corruption_possibility = 100;
    if (!(summon_named_creature(creature_ptr, 0, it_ptr->y, it_ptr->x, it_ptr->q_ptr->pval, !(object_is_cursed(it_ptr->q_ptr)) ? PM_FORCE_PET : 0L))) {
        msg_print(_("人形は捻じ曲がり砕け散ってしまった！", "The Figurine writhes and then shatters."));
        return;
    }

    if (object_is_cursed(it_ptr->q_ptr))
        msg_print(_("これはあまり良くない気がする。", "You have a bad feeling about this."));
}

void display_potion_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if (!object_is_potion(it_ptr->q_ptr))
        return;

    if (it_ptr->hit_body || it_ptr->hit_wall || (randint1(100) < it_ptr->corruption_possibility)) {
        it_ptr->corruption_possibility = 0;
        return;
    }

    msg_format(_("%sは砕け散った！", "The %s shatters!"), it_ptr->o_name);
    if (!potion_smash_effect(creature_ptr, 0, it_ptr->y, it_ptr->x, it_ptr->q_ptr->k_idx)) {
        it_ptr->do_drop = FALSE;
        return;
    }

    monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x].m_idx];
    if ((creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x].m_idx == 0) || !is_friendly(m_ptr) || monster_invulner_remaining(m_ptr)) {
        it_ptr->do_drop = FALSE;
        return;
    }

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(creature_ptr, m_name, m_ptr, 0);
    msg_format(_("%sは怒った！", "%^s gets angry!"), m_name);
    set_hostile(creature_ptr, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x].m_idx]);
    it_ptr->do_drop = FALSE;
}

static void display_boomerang_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if ((it_ptr->back_chance > 37) && !creature_ptr->blind && (it_ptr->item >= 0)) {
        msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), it_ptr->o2_name);
        it_ptr->come_back = TRUE;
        return;
    }

    if (it_ptr->item >= 0)
        msg_format(_("%sを受け損ねた！", "%s comes back, but you can't catch!"), it_ptr->o2_name);
    else
        msg_format(_("%sが返ってきた。", "%s comes back."), it_ptr->o2_name);

    it_ptr->y = creature_ptr->y;
    it_ptr->x = creature_ptr->x;
}

static void process_boomerang_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if ((it_ptr->back_chance <= 30) || (one_in_(100) && !it_ptr->super_boomerang)) {
        msg_format(_("%sが返ってこなかった！", "%s doesn't come back!"), it_ptr->o2_name);
        return;
    }

    for (int i = it_ptr->cur_dis - 1; i > 0; i--) {
        if (!panel_contains(it_ptr->ny[i], it_ptr->nx[i]) || !player_can_see_bold(creature_ptr, it_ptr->ny[i], it_ptr->nx[i])) {
            term_xtra(TERM_XTRA_DELAY, it_ptr->msec);
            continue;
        }

        SYMBOL_CODE c = object_char(it_ptr->q_ptr);
        byte a = object_attr(it_ptr->q_ptr);
        print_rel(creature_ptr, c, a, it_ptr->ny[i], it_ptr->nx[i]);
        move_cursor_relative(it_ptr->ny[i], it_ptr->nx[i]);
        term_fresh();
        term_xtra(TERM_XTRA_DELAY, it_ptr->msec);
        lite_spot(creature_ptr, it_ptr->ny[i], it_ptr->nx[i]);
        term_fresh();
    }

    display_boomerang_throw(creature_ptr, it_ptr);
}

static void check_boomerang_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if (!it_ptr->return_when_thrown)
        return;

    it_ptr->back_chance = randint1(30) + 20 + ((int)(adj_dex_th[creature_ptr->stat_ind[A_DEX]]) - 128);
    it_ptr->super_boomerang = (((it_ptr->q_ptr->name1 == ART_MJOLLNIR) || (it_ptr->q_ptr->name1 == ART_AEGISFANG)) && it_ptr->boomerang);
    it_ptr->corruption_possibility = -1;
    if (it_ptr->boomerang)
        it_ptr->back_chance += 4 + randint1(5);

    if (it_ptr->super_boomerang)
        it_ptr->back_chance += 100;

    describe_flavor(creature_ptr, it_ptr->o2_name, it_ptr->q_ptr, OD_OMIT_PREFIX | OD_NAME_ONLY);
    process_boomerang_throw(creature_ptr, it_ptr);
}

static void process_boomerang_back(player_type *creature_ptr, it_type *it_ptr)
{
    if (it_ptr->come_back) {
        if ((it_ptr->item != INVEN_RARM) && (it_ptr->item != INVEN_LARM)) {
            store_item_to_inventory(creature_ptr, it_ptr->q_ptr);
            it_ptr->do_drop = FALSE;
            return;
        }

        it_ptr->o_ptr = &creature_ptr->inventory_list[it_ptr->item];
        object_copy(it_ptr->o_ptr, it_ptr->q_ptr);
        creature_ptr->equip_cnt++;
        creature_ptr->update |= PU_BONUS | PU_TORCH | PU_MANA;
        creature_ptr->window |= PW_EQUIP;
        it_ptr->do_drop = FALSE;
        return;
    }

    if (it_ptr->equiped_item) {
        verify_equip_slot(creature_ptr, it_ptr->item);
        calc_android_exp(creature_ptr);
    }
}

static void drop_thrown_item(player_type *creature_ptr, it_type *it_ptr)
{
    if (!it_ptr->do_drop)
        return;

    if (cave_has_flag_bold(creature_ptr->current_floor_ptr, it_ptr->y, it_ptr->x, FF_PROJECT))
        (void)drop_near(creature_ptr, it_ptr->q_ptr, it_ptr->corruption_possibility, it_ptr->y, it_ptr->x);
    else
        (void)drop_near(creature_ptr, it_ptr->q_ptr, it_ptr->corruption_possibility, it_ptr->prev_y, it_ptr->prev_x);
}

/*!
 * @brief 投射処理メインルーチン /
 * Throw an object from the pack or floor.
 * @param mult 威力の倍率
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param boomerang ブーメラン処理ならばTRUE
 * @param shuriken 忍者の手裏剣処理ならばTRUE ← 間違い、-1が渡されてくることがある。要調査.
 * @return ターンを消費した場合TRUEを返す
 * @details
 * <pre>
 * Note: "unseen" monsters are very hard to hit.
 *
 * Should throwing a weapon do full damage?  Should it allow the magic
 * to hit bonus of the weapon to have an effect?  Should it ever cause
 * the item to be destroyed?  Should it do any damage at all?
 * </pre>
 */
bool do_cmd_throw(player_type *creature_ptr, int mult, bool boomerang, OBJECT_IDX shuriken)
{
    if (creature_ptr->wild_mode)
        return FALSE;

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    it_type tmp_it;
    object_type tmp_object;
    it_type *it_ptr = initialize_it_type(&tmp_it, &tmp_object, delay_factor, mult, boomerang, shuriken);
    if (!check_can_throw(creature_ptr, it_ptr))
        return FALSE;

    calc_throw_range(creature_ptr, it_ptr);
    if (!calc_throw_grid(creature_ptr, it_ptr))
        return FALSE;

    reflect_inventory_by_throw(creature_ptr, it_ptr);
    if (it_ptr->item >= INVEN_RARM) {
        it_ptr->equiped_item = TRUE;
        creature_ptr->redraw |= PR_EQUIPPY;
    }

    set_class_specific_throw_params(creature_ptr, it_ptr);
    set_racial_chance(creature_ptr, it_ptr);
    it_ptr->prev_y = it_ptr->y;
    it_ptr->prev_x = it_ptr->x;
    exe_throw(creature_ptr, it_ptr);
    if (it_ptr->hit_body)
        torch_lost_fuel(it_ptr->q_ptr);

    it_ptr->corruption_possibility = (it_ptr->hit_body ? breakage_chance(creature_ptr, it_ptr->q_ptr, creature_ptr->pclass == CLASS_ARCHER, 0) : 0);
    display_figurine_throw(creature_ptr, it_ptr);
    display_potion_throw(creature_ptr, it_ptr);
    check_boomerang_throw(creature_ptr, it_ptr);
    process_boomerang_back(creature_ptr, it_ptr);
    drop_thrown_item(creature_ptr, it_ptr);
    return TRUE;
}
