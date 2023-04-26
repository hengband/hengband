/*!
 * @brief 自動破壊の実行
 * @date 2020/04/25
 * @author Hourier
 */

#include "autopick/autopick-destroyer.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "flavor/flavor-describer.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/input-options.h"
#include "monster-race/monster-race.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-quest.h"
#include "object-hook/hook-weapon.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-wand-types.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief クラス依存のアイテム破壊を調べる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr アイテムへの参照ポインタ
 * @return 特別なクラス、かつそのクラス特有のアイテムであればFALSE、それ以外はTRUE
 */
static bool is_leave_special_item(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (!leave_special) {
        return true;
    }

    PlayerClass pc(player_ptr);
    const auto &bi_key = o_ptr->bi_key;
    const auto tval = bi_key.tval();
    if (PlayerRace(player_ptr).equals(PlayerRaceType::BALROG)) {
        auto r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
        if (bi_key == BaseitemKey(ItemKindType::CORPSE, SV_CORPSE) && angband_strchr("pht", monraces_info[r_idx].d_char)) {
            return false;
        }
    } else if (pc.equals(PlayerClassType::ARCHER)) {
        if ((tval == ItemKindType::SKELETON) || (bi_key == BaseitemKey(ItemKindType::CORPSE, SV_SKELETON))) {
            return false;
        }
    } else if (pc.equals(PlayerClassType::NINJA)) {
        if (tval == ItemKindType::LITE && o_ptr->ego_idx == EgoType::LITE_DARKNESS && o_ptr->is_known()) {
            return false;
        }
    } else if (pc.is_tamer()) {
        if (bi_key == BaseitemKey(ItemKindType::WAND, SV_WAND_HEAL_MONSTER) && o_ptr->is_aware()) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief Automatically destroy items in this grid.
 */
static bool is_opt_confirm_destroy(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (!destroy_items) {
        return false;
    }

    if (leave_worth) {
        if (o_ptr->get_price() > 0) {
            return false;
        }
    }

    if (leave_equip) {
        if (o_ptr->is_weapon_armour_ammo()) {
            return false;
        }
    }

    const auto tval = o_ptr->bi_key.tval();
    if (leave_chest) {
        if ((tval == ItemKindType::CHEST) && o_ptr->pval) {
            return false;
        }
    }

    if (leave_wanted) {
        if (object_is_bounty(player_ptr, o_ptr)) {
            return false;
        }
    }

    if (leave_corpse) {
        if (tval == ItemKindType::CORPSE) {
            return false;
        }
    }

    if (leave_junk) {
        if (o_ptr->is_junk()) {
            return false;
        }
    }

    if (!is_leave_special_item(player_ptr, o_ptr)) {
        return false;
    }

    if (tval == ItemKindType::GOLD) {
        return false;
    }

    return true;
}

void auto_destroy_item(PlayerType *player_ptr, ItemEntity *o_ptr, int autopick_idx)
{
    bool destroy = false;
    if (is_opt_confirm_destroy(player_ptr, o_ptr)) {
        destroy = true;
    }

    if (autopick_idx >= 0 && !(autopick_list[autopick_idx].action & DO_AUTODESTROY)) {
        destroy = false;
    }

    if (!always_pickup) {
        if (autopick_idx >= 0 && (autopick_list[autopick_idx].action & DO_AUTODESTROY)) {
            destroy = true;
        }
    }

    if (!destroy) {
        return;
    }

    disturb(player_ptr, false, false);
    if (!can_player_destroy_object(player_ptr, o_ptr)) {
        const auto item_name = describe_flavor(player_ptr, o_ptr, 0);
        msg_format(_("%sは破壊不能だ。", "You cannot auto-destroy %s."), item_name.data());
        return;
    }

    autopick_last_destroyed_object = *o_ptr;
    o_ptr->marked.set(OmType::AUTODESTROY);
    player_ptr->update |= PU_AUTODESTROY;
}
