/*
 * @brief アイテム投擲コマンドの実装
 * @date 2021/08/20
 * @author Hourier
 */
#include "cmd-item/cmd-throw.h"
#include "core/player-redraw-types.h"
#include "game-option/special-options.h"
#include "inventory/inventory-slot-types.h"
#include "object-use/throw-execution.h"
#include "object/object-broken.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "specific-object/torch.h"
#include "status/action-setter.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

ThrowCommand::ThrowCommand(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 投射処理メインルーチン /
 * Throw an object from the pack or floor.
 * @param mult 威力の倍率
 * @param player_ptr プレイヤーへの参照ポインタ
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
    if (this->player_ptr->wild_mode) {
        return false;
    }

    PlayerClass pc(this->player_ptr);
    pc.break_samurai_stance({ SamuraiStanceType::MUSOU });

    ObjectType tmp_object;
    ObjectThrowEntity ote(this->player_ptr, &tmp_object, delay_factor, mult, boomerang, shuriken);
    if (!ote.check_can_throw()) {
        return false;
    }

    ote.calc_throw_range();
    if (!ote.calc_throw_grid()) {
        return false;
    }

    ote.reflect_inventory_by_throw();
    if (ote.item >= INVEN_MAIN_HAND) {
        ote.equiped_item = true;
        this->player_ptr->redraw |= PR_EQUIPPY;
    }

    ote.set_class_specific_throw_params();
    ote.set_racial_chance();
    ote.prev_y = ote.y;
    ote.prev_x = ote.x;
    ote.exe_throw();
    if (ote.hit_body) {
        torch_lost_fuel(ote.q_ptr);
    }

    ote.corruption_possibility = ote.hit_body ? breakage_chance(this->player_ptr, ote.q_ptr, pc.equals(PlayerClassType::ARCHER), 0) : 0;
    ote.display_figurine_throw();
    ote.display_potion_throw();
    ote.check_boomerang_throw();
    ote.process_boomerang_back();
    ote.drop_thrown_item();
    return true;
}
