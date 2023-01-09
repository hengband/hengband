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
    const auto tval = o_ptr->bi_key.tval();
    const auto sval = o_ptr->bi_key.sval().value();
    switch (player_ptr->pclass) {
    case PlayerClassType::PRIEST: {
        const auto flags = object_flags_known(o_ptr);
        return flags.has(TR_BLESSED) || (tval == ItemKindType::HAFTED);
    }
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
        /* Icky to wield? */
        return player_ptr->weapon_exp_max[tval][sval] != PlayerSkill::weapon_exp_at(PlayerSkillRank::UNSKILLED);
    case PlayerClassType::BEASTMASTER:
    case PlayerClassType::CAVALRY: {
        /* Is it known to be suitable to using while riding? */
        auto flags = object_flags_known(o_ptr);
        return flags.has(TR_RIDING);
    }
    case PlayerClassType::SORCERER:
        return player_ptr->weapon_exp_max[tval][sval] >= PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER);
    case PlayerClassType::NINJA:
        /* Icky to wield? */
        return player_ptr->weapon_exp_max[tval][sval] > PlayerSkill::weapon_exp_at(PlayerSkillRank::BEGINNER);
    default:
        return true;
    }
}
