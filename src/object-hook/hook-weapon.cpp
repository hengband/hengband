#include "object-hook/hook-weapon.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "player/player-skill.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトがプレイヤーの職業に応じた適正武器か否かを返す / Favorite weapons
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return オブジェクトが適正武器ならばTRUEを返す
 */
bool object_is_favorite(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (!o_ptr->is_melee_weapon()) {
        return false;
    }

    /* Favorite weapons are varied depend on the class */
    switch (player_ptr->pclass) {
    case PlayerClassType::PRIEST: {
        auto flgs = object_flags_known(o_ptr);

        if (flgs.has_not(TR_BLESSED) && !(o_ptr->tval == ItemKindType::HAFTED)) {
            return false;
        }
        break;
    }

    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
        /* Icky to wield? */
        if (player_ptr->weapon_exp_max[o_ptr->tval][o_ptr->sval] == PlayerSkill::weapon_exp_at(PlayerSkillRank::UNSKILLED)) {
            return false;
        }
        break;

    case PlayerClassType::BEASTMASTER:
    case PlayerClassType::CAVALRY: {
        auto flgs = object_flags_known(o_ptr);

        /* Is it known to be suitable to using while riding? */
        if (flgs.has_not(TR_RIDING)) {
            return false;
        }

        break;
    }

    case PlayerClassType::SORCERER:
        if (player_ptr->weapon_exp_max[o_ptr->tval][o_ptr->sval] < PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER)) {
            return false;
        }
        break;

    case PlayerClassType::NINJA:
        /* Icky to wield? */
        if (player_ptr->weapon_exp_max[o_ptr->tval][o_ptr->sval] <= PlayerSkill::weapon_exp_at(PlayerSkillRank::BEGINNER)) {
            return false;
        }
        break;

    default:
        /* All weapons are okay for non-special classes */
        return true;
    }

    return true;
}
