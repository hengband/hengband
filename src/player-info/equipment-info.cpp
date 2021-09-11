#include "player-info/equipment-info.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-weapon.h"
#include "pet/pet-util.h"
#include "player-status/player-hand-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーが現在右手/左手に武器を持っているか判定する /
 * @param i 判定する手のID(右手:INVEN_MAIN_HAND 左手:INVEN_SUB_HAND)
 * @return 持っているならばTRUE
 */
bool has_melee_weapon(player_type *player_ptr, int slot)
{
    const auto o_ptr = &player_ptr->inventory_list[slot];
    return o_ptr->k_idx && o_ptr->is_melee_weapon();
}

/*!
 * @brief プレイヤーの現在開いている手の状態を返す
 * @param riding_control 乗馬中により片手を必要としている状態ならばTRUEを返す。
 * @return 開いている手のビットフラグ
 */
BIT_FLAGS16 empty_hands(player_type *player_ptr, bool riding_control)
{
    BIT_FLAGS16 status = EMPTY_HAND_NONE;
    if (!player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx)
        status |= EMPTY_HAND_MAIN;
    if (!player_ptr->inventory_list[INVEN_SUB_HAND].k_idx)
        status |= EMPTY_HAND_SUB;

    if (riding_control && (status != EMPTY_HAND_NONE) && player_ptr->riding && none_bits(player_ptr->pet_extra_flags, PF_TWO_HANDS)) {
        if (any_bits(status, EMPTY_HAND_SUB))
            reset_bits(status, EMPTY_HAND_SUB);
        else if (any_bits(status, EMPTY_HAND_MAIN))
            reset_bits(status, EMPTY_HAND_MAIN);
    }

    return status;
}

bool can_two_hands_wielding(player_type *player_ptr)
{
    return !player_ptr->riding || any_bits(player_ptr->pet_extra_flags, PF_TWO_HANDS);
}

/*!
 * @brief プレイヤーが防具重量制限のある職業時にペナルティを受ける状態にあるかどうかを返す。
 * @return ペナルティが適用されるならばTRUE。
 */
bool heavy_armor(player_type *player_ptr)
{
    if ((player_ptr->pclass != CLASS_MONK) && (player_ptr->pclass != CLASS_FORCETRAINER) && (player_ptr->pclass != CLASS_NINJA))
        return false;

    WEIGHT monk_arm_wgt = 0;
    if (player_ptr->inventory_list[INVEN_MAIN_HAND].tval > TV_SWORD)
        monk_arm_wgt += player_ptr->inventory_list[INVEN_MAIN_HAND].weight;
    if (player_ptr->inventory_list[INVEN_SUB_HAND].tval > TV_SWORD)
        monk_arm_wgt += player_ptr->inventory_list[INVEN_SUB_HAND].weight;
    monk_arm_wgt += player_ptr->inventory_list[INVEN_BODY].weight;
    monk_arm_wgt += player_ptr->inventory_list[INVEN_HEAD].weight;
    monk_arm_wgt += player_ptr->inventory_list[INVEN_OUTER].weight;
    monk_arm_wgt += player_ptr->inventory_list[INVEN_ARMS].weight;
    monk_arm_wgt += player_ptr->inventory_list[INVEN_FEET].weight;

    return (monk_arm_wgt > (100 + (player_ptr->lev * 4)));
}
