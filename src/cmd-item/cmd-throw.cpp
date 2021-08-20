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
    it_ptr->display_figurine_throw();
    it_ptr->display_potion_throw();
    it_ptr->check_boomerang_throw();
    it_ptr->process_boomerang_back();
    drop_thrown_item(this->creature_ptr, it_ptr);
    return true;
}
