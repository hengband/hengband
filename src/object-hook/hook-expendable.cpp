#include "object-hook/hook-expendable.h"
#include "artifact/fixed-art-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "monster-race/monster-race.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"

/*!
 * @brief オブジェクトをプレイヤーが食べることができるかを判定する /
 * Hook to determine if an object is eatable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 食べることが可能ならばTRUEを返す
 */
bool item_tester_hook_eatable(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (o_ptr->tval == ItemKindType::FOOD) {
        return true;
    }

    auto food_type = PlayerRace(player_ptr).food();
    if (food_type == PlayerRaceFoodType::MANA) {
        if (o_ptr->tval == ItemKindType::STAFF || o_ptr->tval == ItemKindType::WAND) {
            return true;
        }
    } else if (food_type == PlayerRaceFoodType::CORPSE) {
        auto corpse_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        if (o_ptr->tval == ItemKindType::CORPSE && o_ptr->sval == SV_CORPSE && angband_strchr("pht", monraces_info[corpse_r_idx].d_char)) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief オブジェクトをプレイヤーが飲むことができるかを判定する /
 * Hook to determine if an object can be quaffed
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 飲むことが可能ならばTRUEを返す
 */
bool item_tester_hook_quaff(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (o_ptr->tval == ItemKindType::POTION) {
        return true;
    }

    if (PlayerRace(player_ptr).food() == PlayerRaceFoodType::OIL && o_ptr->tval == ItemKindType::FLASK && o_ptr->sval == SV_FLASK_OIL) {
        return true;
    }

    return false;
}

/*!
 * @brief 破壊可能なアイテムかを返す /
 * Determines whether an object can be destroyed, and makes fake inscription.
 * @param o_ptr 破壊可能かを確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが破壊可能ならばTRUEを返す
 */
bool can_player_destroy_object(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    /* Artifacts cannot be destroyed */
    if (!o_ptr->is_artifact()) {
        return true;
    }

    if (!o_ptr->is_known()) {
        byte feel = FEEL_SPECIAL;
        if (o_ptr->is_cursed() || o_ptr->is_broken()) {
            feel = FEEL_TERRIBLE;
        }

        o_ptr->feeling = feel;
        o_ptr->ident |= IDENT_SENSE;
        player_ptr->update |= (PU_COMBINE);
        player_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
        return false;
    }

    return false;
}
