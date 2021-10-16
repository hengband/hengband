#include "object-hook/hook-weapon.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object/object-flags.h"
#include "player/player-skill.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトがプレイヤーの職業に応じた適正武器か否かを返す / Favorite weapons
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが適正武器ならばTRUEを返す
 */
bool object_is_favorite(player_type *player_ptr, const object_type *o_ptr)
{
    /* Only melee weapons match */
    if (!(o_ptr->tval == ItemKindType::POLEARM || o_ptr->tval == ItemKindType::SWORD || o_ptr->tval == ItemKindType::DIGGING || o_ptr->tval == ItemKindType::HAFTED)) {
        return false;
    }

    /* Favorite weapons are varied depend on the class */
    auto short_pclass = enum2i(player_ptr->pclass);
    switch (player_ptr->pclass) {
    case PlayerClassType::PRIEST: {
        auto flgs = object_flags_known(o_ptr);

        if (flgs.has_not(TR_BLESSED) && !(o_ptr->tval == ItemKindType::HAFTED))
            return false;
        break;
    }

    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
        /* Icky to wield? */
        if (!(s_info[short_pclass].w_max[o_ptr->tval][o_ptr->sval]))
            return false;
        break;

    case PlayerClassType::BEASTMASTER:
    case PlayerClassType::CAVALRY: {
        auto flgs = object_flags_known(o_ptr);

        /* Is it known to be suitable to using while riding? */
        if (flgs.has_not(TR_RIDING))
            return false;

        break;
    }

    case PlayerClassType::SORCERER:
        if (s_info[short_pclass].w_max[o_ptr->tval][o_ptr->sval] < PlayerSkill::weapon_exp_at(EXP_LEVEL_MASTER))
            return false;
        break;

    case PlayerClassType::NINJA:
        /* Icky to wield? */
        if (s_info[short_pclass].w_max[o_ptr->tval][o_ptr->sval] <= PlayerSkill::weapon_exp_at(EXP_LEVEL_BEGINNER))
            return false;
        break;

    default:
        /* All weapons are okay for non-special classes */
        return true;
    }

    return true;
}
