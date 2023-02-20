#include "spell-kind/spells-equipment.h"
#include "avatar/avatar.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "object-hook/hook-weapon.h"
#include "object/object-info.h"
#include "racial/racial-android.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief プレイヤーの装備劣化処理 /
 * Apply disenchantment to the player's stuff
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mode 最下位ビットが1ならば劣化処理が若干低減される
 * @return 劣化処理に関するメッセージが発せられた場合はTRUEを返す /
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(PlayerType *player_ptr, BIT_FLAGS mode)
{
    int t = 0;
    switch (randint1(8)) {
    case 1:
        t = INVEN_MAIN_HAND;
        break;
    case 2:
        t = INVEN_SUB_HAND;
        break;
    case 3:
        t = INVEN_BOW;
        break;
    case 4:
        t = INVEN_BODY;
        break;
    case 5:
        t = INVEN_OUTER;
        break;
    case 6:
        t = INVEN_HEAD;
        break;
    case 7:
        t = INVEN_ARMS;
        break;
    case 8:
        t = INVEN_FEET;
        break;
    }

    ItemEntity *o_ptr;
    o_ptr = &player_ptr->inventory_list[t];
    if (!o_ptr->bi_id) {
        return false;
    }

    if (!o_ptr->is_weapon_armour_ammo()) {
        return false;
    }

    if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0) && (o_ptr->pval <= 1)) {
        return false;
    }

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (o_ptr->is_fixed_or_random_artifact() && (randint0(100) < 71)) {
#ifdef JP
        msg_format("%s(%c)は劣化を跳ね返した！", o_name, index_to_label(t));
#else
        msg_format("Your %s (%c) resist%s disenchantment!", o_name, index_to_label(t), ((o_ptr->number != 1) ? "" : "s"));
#endif
        return true;
    }

    int to_h = o_ptr->to_h;
    int to_d = o_ptr->to_d;
    int to_a = o_ptr->to_a;
    int pval = o_ptr->pval;

    if (o_ptr->to_h > 0) {
        o_ptr->to_h--;
    }
    if ((o_ptr->to_h > 5) && (randint0(100) < 20)) {
        o_ptr->to_h--;
    }

    if (o_ptr->to_d > 0) {
        o_ptr->to_d--;
    }
    if ((o_ptr->to_d > 5) && (randint0(100) < 20)) {
        o_ptr->to_d--;
    }

    if (o_ptr->to_a > 0) {
        o_ptr->to_a--;
    }
    if ((o_ptr->to_a > 5) && (randint0(100) < 20)) {
        o_ptr->to_a--;
    }

    if ((o_ptr->pval > 1) && one_in_(13) && !(mode & 0x01)) {
        o_ptr->pval--;
    }

    bool is_actually_disenchanted = to_h != o_ptr->to_h;
    is_actually_disenchanted |= to_d != o_ptr->to_d;
    is_actually_disenchanted |= to_a != o_ptr->to_a;
    is_actually_disenchanted |= pval != o_ptr->pval;
    if (!is_actually_disenchanted) {
        return true;
    }

#ifdef JP
    msg_format("%s(%c)は劣化してしまった！", o_name, index_to_label(t));
#else
    msg_format("Your %s (%c) %s disenchanted!", o_name, index_to_label(t), ((o_ptr->number != 1) ? "were" : "was"));
#endif
    chg_virtue(player_ptr, V_HARMONY, 1);
    chg_virtue(player_ptr, V_ENCHANT, -2);
    player_ptr->update |= (PU_BONUS);
    player_ptr->window_flags |= (PW_EQUIP | PW_PLAYER);

    calc_android_exp(player_ptr);
    return true;
}
