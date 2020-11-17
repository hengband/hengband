#include "object-enchant/object-curse.h"
#include "core/player-update-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

#define MAX_CURSE 18
#define TRC_SPECIAL_MASK (TRC_TY_CURSE | TRC_AGGRAVATE)
#define TRC_HEAVY_MASK (TRC_TY_CURSE | TRC_AGGRAVATE | TRC_DRAIN_EXP | TRC_ADD_H_CURSE | TRC_CALL_DEMON | TRC_CALL_DRAGON | TRC_CALL_UNDEAD | TRC_TELEPORT)

/*!
 * @brief アイテムに付加される可能性のある呪いを指定する。
 * @param power 呪いの段階
 * @param o_ptr 呪いをかけられる装備オブジェクトの構造体参照ポインタ
 * @return 与える呪いのID
 */
BIT_FLAGS get_curse(player_type *owner_ptr, int power, object_type *o_ptr)
{
    BIT_FLAGS new_curse;

    while (TRUE) {
        new_curse = (1 << (randint0(MAX_CURSE) + 4));
        if (power == 2) {
            if (!(new_curse & TRC_HEAVY_MASK))
                continue;
        } else if (power == 1) {
            if (new_curse & TRC_SPECIAL_MASK)
                continue;
        } else if (power == 0) {
            if (new_curse & TRC_HEAVY_MASK)
                continue;
        }

        if (new_curse == TRC_LOW_MELEE && !object_is_weapon(owner_ptr, o_ptr))
            continue;
        if (new_curse == TRC_LOW_AC && !object_is_armour(owner_ptr, o_ptr))
            continue;
        break;
    }

    return new_curse;
}

/*!
 * @brief 装備への呪い付加判定と付加処理
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param chance 呪いの基本確率
 * @param heavy_chance さらに重い呪いとなる確率
 * @return なし
 */
void curse_equipment(player_type *owner_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance)
{
    if (randint1(100) > chance)
        return;

    object_type *o_ptr = &owner_ptr->inventory_list[INVEN_RARM + randint0(12)];
    if (!o_ptr->k_idx)
        return;
    BIT_FLAGS oflgs[TR_FLAG_SIZE];
    object_flags(owner_ptr, o_ptr, oflgs);
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(owner_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    /* Extra, biased saving throw for blessed items */
    if (has_flag(oflgs, TR_BLESSED)) {
#ifdef JP
        msg_format("祝福された%sは呪いを跳ね返した！", o_name);
#else
        msg_format("Your blessed %s resist%s cursing!", o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
        /* Hmmm -- can we wear multiple items? If not, this is unnecessary */
        return;
    }

    bool changed = FALSE;
    int curse_power = 0;
    if ((randint1(100) <= heavy_chance) && (object_is_artifact(o_ptr) || object_is_ego(o_ptr))) {
        if (!(o_ptr->curse_flags & TRC_HEAVY_CURSE))
            changed = TRUE;
        o_ptr->curse_flags |= TRC_HEAVY_CURSE;
        o_ptr->curse_flags |= TRC_CURSED;
        curse_power++;
    } else {
        if (!object_is_cursed(o_ptr))
            changed = TRUE;
        o_ptr->curse_flags |= TRC_CURSED;
    }

    if (heavy_chance >= 50)
        curse_power++;

    BIT_FLAGS new_curse = get_curse(owner_ptr, curse_power, o_ptr);
    if (!(o_ptr->curse_flags & new_curse)) {
        changed = TRUE;
        o_ptr->curse_flags |= new_curse;
    }

    if (changed) {
        msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding %s..."), o_name);
        o_ptr->feeling = FEEL_NONE;
    }

    owner_ptr->update |= PU_BONUS;
}
