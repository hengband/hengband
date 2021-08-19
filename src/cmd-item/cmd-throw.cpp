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
#include "player/attack-defense-types.h"
#include "player/player-status-table.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "specific-object/torch.h"
#include "status/action-setter.h"
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

ThrowCommand::ThrowCommand(player_type* creature_ptr)
    : creature_ptr(creature_ptr)
{
}

void display_figurine_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if ((it_ptr->q_ptr->tval != TV_FIGURINE) || creature_ptr->current_floor_ptr->inside_arena)
        return;

    it_ptr->corruption_possibility = 100;
    if (!(summon_named_creature(creature_ptr, 0, it_ptr->y, it_ptr->x, it_ptr->q_ptr->pval, !(object_is_cursed(it_ptr->q_ptr)) ? PM_FORCE_PET : PM_NONE))) {
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

    if (!it_ptr->hit_body && !it_ptr->hit_wall && (randint1(100) >= it_ptr->corruption_possibility)) {
        it_ptr->corruption_possibility = 0;
        return;
    }

    msg_format(_("%sは砕け散った！", "The %s shatters!"), it_ptr->o_name);
    if (!potion_smash_effect(creature_ptr, 0, it_ptr->y, it_ptr->x, it_ptr->q_ptr->k_idx)) {
        it_ptr->do_drop = false;
        return;
    }

    monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x].m_idx];
    if ((creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x].m_idx == 0) || !is_friendly(m_ptr) || monster_invulner_remaining(m_ptr)) {
        it_ptr->do_drop = false;
        return;
    }

    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(creature_ptr, m_name, m_ptr, 0);
    msg_format(_("%sは怒った！", "%^s gets angry!"), m_name);
    set_hostile(creature_ptr, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[it_ptr->y][it_ptr->x].m_idx]);
    it_ptr->do_drop = false;
}

static void display_boomerang_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if ((it_ptr->back_chance > 37) && !creature_ptr->blind && (it_ptr->item >= 0)) {
        msg_format(_("%sが手元に返ってきた。", "%s comes back to you."), it_ptr->o2_name);
        it_ptr->come_back = true;
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

        if (it_ptr->msec > 0) {
            print_rel(creature_ptr, c, a, it_ptr->ny[i], it_ptr->nx[i]);
            move_cursor_relative(it_ptr->ny[i], it_ptr->nx[i]);
            term_fresh();
            term_xtra(TERM_XTRA_DELAY, it_ptr->msec);
            lite_spot(creature_ptr, it_ptr->ny[i], it_ptr->nx[i]);
            term_fresh();
        }
    }

    display_boomerang_throw(creature_ptr, it_ptr);
}

static void check_boomerang_throw(player_type *creature_ptr, it_type *it_ptr)
{
    if (!it_ptr->return_when_thrown)
        return;

    it_ptr->back_chance = randint1(30) + 20 + ((int)(adj_dex_th[creature_ptr->stat_index[A_DEX]]) - 128);
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
        if ((it_ptr->item != INVEN_MAIN_HAND) && (it_ptr->item != INVEN_SUB_HAND)) {
            store_item_to_inventory(creature_ptr, it_ptr->q_ptr);
            it_ptr->do_drop = false;
            return;
        }

        it_ptr->o_ptr = &creature_ptr->inventory_list[it_ptr->item];
        it_ptr->o_ptr->copy_from(it_ptr->q_ptr);
        creature_ptr->equip_cnt++;
        creature_ptr->update |= PU_BONUS | PU_TORCH | PU_MANA;
        creature_ptr->window_flags |= PW_EQUIP;
        it_ptr->do_drop = false;
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
bool ThrowCommand::do_cmd_throw(int mult, bool boomerang, OBJECT_IDX shuriken)
{
    if (this->creature_ptr->wild_mode)
        return false;

    if (this->creature_ptr->special_defense & KATA_MUSOU)
        set_action(this->creature_ptr, ACTION_NONE);

    object_type tmp_object;
    it_type tmp_it(this->creature_ptr, &tmp_object, delay_factor, mult, boomerang, shuriken);
    it_type *it_ptr = &tmp_it;
    if (!it_ptr->check_can_throw())
        return false;

    it_ptr->calc_throw_range();
    if (!it_ptr->calc_throw_grid())
        return false;

    it_ptr->reflect_inventory_by_throw();
    if (it_ptr->item >= INVEN_MAIN_HAND) {
        it_ptr->equiped_item = true;
        this->creature_ptr->redraw |= PR_EQUIPPY;
    }

    it_ptr->set_class_specific_throw_params();
    it_ptr->set_racial_chance();
    it_ptr->prev_y = it_ptr->y;
    it_ptr->prev_x = it_ptr->x;
    it_ptr->exe_throw();
    if (it_ptr->hit_body)
        torch_lost_fuel(it_ptr->q_ptr);

    it_ptr->corruption_possibility = (it_ptr->hit_body ? breakage_chance(this->creature_ptr, it_ptr->q_ptr, this->creature_ptr->pclass == CLASS_ARCHER, 0) : 0);
    display_figurine_throw(this->creature_ptr, it_ptr);
    display_potion_throw(this->creature_ptr, it_ptr);
    check_boomerang_throw(this->creature_ptr, it_ptr);
    process_boomerang_back(this->creature_ptr, it_ptr);
    drop_thrown_item(this->creature_ptr, it_ptr);
    return true;
}
