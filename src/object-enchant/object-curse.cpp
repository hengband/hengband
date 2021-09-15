#include "object-enchant/object-curse.h"
#include "core/player-update-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"

namespace {
const EnumClassFlagGroup<TRC> TRC_SPECIAL_MASK({TRC::TY_CURSE, TRC::AGGRAVATE});
const EnumClassFlagGroup<TRC> TRC_HEAVY_MASK({TRC::TY_CURSE, TRC::AGGRAVATE, TRC::DRAIN_EXP, TRC::ADD_H_CURSE, TRC::CALL_DEMON, TRC::CALL_DRAGON, TRC::CALL_UNDEAD, TRC::TELEPORT});
}

/*!
 * @brief アイテムに付加される可能性のある呪いを指定する。
 * @param power 呪いの段階
 * @param o_ptr 呪いをかけられる装備オブジェクトの構造体参照ポインタ
 * @return 与える呪いのID
 */
TRC get_curse(int power, object_type *o_ptr)
{
    TRC new_curse;

    while (true) {
        new_curse = i2enum<TRC>(rand_range(enum2i(TRC::TY_CURSE), enum2i(TRC::MAX) - 1));
        if (power == 2) {
            if (TRC_HEAVY_MASK.has_not(new_curse))
                continue;
        } else if (power == 1) {
            if (TRC_SPECIAL_MASK.has(new_curse))
                continue;
        } else if (power == 0) {
            if (TRC_HEAVY_MASK.has(new_curse))
                continue;
        }

        if (new_curse == TRC::LOW_MELEE && !o_ptr->is_weapon())
            continue;
        if (new_curse == TRC::LOW_AC && !o_ptr->is_armour())
            continue;
        break;
    }

    return new_curse;
}

/*!
 * @brief 装備への呪い付加判定と付加処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param chance 呪いの基本確率
 * @param heavy_chance さらに重い呪いとなる確率
 */
void curse_equipment(player_type *player_ptr, PERCENTAGE chance, PERCENTAGE heavy_chance)
{
    if (randint1(100) > chance)
        return;

    object_type *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + randint0(12)];
    if (!o_ptr->k_idx)
        return;
    auto oflgs = object_flags(o_ptr);
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

    /* Extra, biased saving throw for blessed items */
    if (oflgs.has(TR_BLESSED)) {
#ifdef JP
        msg_format("祝福された%sは呪いを跳ね返した！", o_name);
#else
        msg_format("Your blessed %s resist%s cursing!", o_name, ((o_ptr->number > 1) ? "" : "s"));
#endif
        /* Hmmm -- can we wear multiple items? If not, this is unnecessary */
        return;
    }

    bool changed = false;
    int curse_power = 0;
    if ((randint1(100) <= heavy_chance) && (o_ptr->is_artifact() || o_ptr->is_ego())) {
        if (o_ptr->curse_flags.has_not(TRC::HEAVY_CURSE))
            changed = true;
        o_ptr->curse_flags.set(TRC::HEAVY_CURSE);
        o_ptr->curse_flags.set(TRC::CURSED);
        curse_power++;
    } else {
        if (!o_ptr->is_cursed())
            changed = true;
        o_ptr->curse_flags.set(TRC::CURSED);
    }

    if (heavy_chance >= 50)
        curse_power++;

    auto new_curse = get_curse(curse_power, o_ptr);
    if (o_ptr->curse_flags.has_not(new_curse)) {
        changed = true;
        o_ptr->curse_flags.set(new_curse);
    }

    if (changed) {
        msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding %s..."), o_name);
        o_ptr->feeling = FEEL_NONE;
    }

    player_ptr->update |= PU_BONUS;
}
