#include "object-hook/hook-expendable.h"
#include "artifact/fixed-art-types.h"
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
#include "system/redrawing-flags-updater.h"
#include "util/string-processor.h"

/*!
 * @brief オブジェクトをプレイヤーが食べることができるかを判定する /
 * Hook to determine if an object is eatable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 食べることが可能ならばTRUEを返す
 */
bool item_tester_hook_eatable(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    const auto tval = o_ptr->bi_key.tval();
    if (tval == ItemKindType::FOOD) {
        return true;
    }

    auto food_type = PlayerRace(player_ptr).food();
    if (food_type == PlayerRaceFoodType::MANA) {
        if (o_ptr->is_wand_staff()) {
            return true;
        }
    } else if (food_type == PlayerRaceFoodType::CORPSE) {
        auto corpse_r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        if ((o_ptr->bi_key == BaseitemKey(ItemKindType::CORPSE, SV_CORPSE)) && angband_strchr("pht", monraces_info[corpse_r_idx].d_char)) {
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
    const auto &bi_key = o_ptr->bi_key;
    if (bi_key.tval() == ItemKindType::POTION) {
        return true;
    }

    return (PlayerRace(player_ptr).food() == PlayerRaceFoodType::OIL) && (bi_key == BaseitemKey(ItemKindType::FLASK, SV_FLASK_OIL));
}

/*!
 * @brief 破壊可能なアイテムかを返す
 * @param o_ptr 破壊可能かを確認したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが破壊可能ならばTRUEを返す
 */
bool can_player_destroy_object(ItemEntity *o_ptr)
{
    /* Artifacts cannot be destroyed */
    if (!o_ptr->is_fixed_or_random_artifact()) {
        return true;
    }

    if (!o_ptr->is_known()) {
        byte feel = FEEL_SPECIAL;
        if (o_ptr->is_cursed() || o_ptr->is_broken()) {
            feel = FEEL_TERRIBLE;
        }

        o_ptr->feeling = feel;
        o_ptr->ident |= IDENT_SENSE;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(StatusRecalculatingFlag::COMBINATION);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::INVENTORY,
            SubWindowRedrawingFlag::EQUIPMENT,
        };
        rfu.set_flags(flags);
        return false;
    }

    return false;
}
