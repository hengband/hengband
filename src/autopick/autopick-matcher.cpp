/*!
 * @brief 床のアイテムが自動拾いに一致するかどうかを調べる関数だけを格納したファイル
 * @date 2020/04/25
 * @author Hourier
 * @todo 300行以上の凶悪関数なので後で分割しておく
 */

#include "autopick/autopick-matcher.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-key-flag-process.h"
#include "autopick/autopick-util.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-quest.h"
#include "object-hook/hook-weapon.h"
#include "object/object-info.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player/player-realm.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"

static bool check_item_features(PlayerType *player_ptr, const autopick_type &entry, const ItemEntity &item, const ItemKindType tval)
{
    if (entry.has(FLG_WEAPONS)) {
        return item.is_weapon();
    }

    if (entry.has(FLG_FAVORITE_WEAPONS)) {
        return object_is_favorite(player_ptr, &item);
    }

    if (entry.has(FLG_ARMORS)) {
        return item.is_protector();
    }

    if (entry.has(FLG_MISSILES)) {
        return item.is_ammo();
    }

    if (entry.has(FLG_DEVICES)) {
        switch (tval) {
        case ItemKindType::SCROLL:
        case ItemKindType::STAFF:
        case ItemKindType::WAND:
        case ItemKindType::ROD:
            return true;
        default:
            return false;
        }
    }

    if (entry.has(FLG_LIGHTS)) {
        return tval == ItemKindType::LITE;
    }

    if (entry.has(FLG_JUNKS)) {
        switch (tval) {
        case ItemKindType::SKELETON:
        case ItemKindType::BOTTLE:
        case ItemKindType::JUNK:
        case ItemKindType::STATUE:
            return true;
        default:
            return false;
        }
    }

    if (entry.has(FLG_CORPSES)) {
        return (tval == ItemKindType::CORPSE) || (tval == ItemKindType::SKELETON);
    }

    if (entry.has(FLG_SPELLBOOKS)) {
        return item.is_spell_book();
    }

    if (entry.has(FLG_HAFTED)) {
        return tval == ItemKindType::HAFTED;
    }

    if (entry.has(FLG_SHIELDS)) {
        return tval == ItemKindType::SHIELD;
    }

    if (entry.has(FLG_BOWS)) {
        return tval == ItemKindType::BOW;
    }

    if (entry.has(FLG_RINGS)) {
        return tval == ItemKindType::RING;
    }

    if (entry.has(FLG_AMULETS)) {
        return tval == ItemKindType::AMULET;
    }

    if (entry.has(FLG_SUITS)) {
        return item.is_armour();
    }

    if (entry.has(FLG_CLOAKS)) {
        return tval == ItemKindType::CLOAK;
    }

    if (entry.has(FLG_HELMS)) {
        return (tval != ItemKindType::CROWN) && (tval != ItemKindType::HELM);
    }

    if (entry.has(FLG_GLOVES)) {
        return tval == ItemKindType::GLOVES;
    }

    if (entry.has(FLG_BOOTS)) {
        return tval == ItemKindType::BOOTS;
    }

    return true;
}

/*!
 * @brief A function for Auto-picker/destroyer Examine whether the object matches to the entry
 */
bool is_autopick_match(PlayerType *player_ptr, ItemEntity *o_ptr, const autopick_type &entry, std::string_view item_name)
{
    if (entry.has(FLG_UNAWARE) && o_ptr->is_aware()) {
        return false;
    }

    if (entry.has(FLG_UNIDENTIFIED) && (o_ptr->is_known() || (o_ptr->ident & IDENT_SENSE))) {
        return false;
    }

    if (entry.has(FLG_IDENTIFIED) && !o_ptr->is_known()) {
        return false;
    }

    if (entry.has(FLG_STAR_IDENTIFIED) && (!o_ptr->is_known() || !o_ptr->is_fully_known())) {
        return false;
    }

    if (entry.has(FLG_BOOSTED)) {
        if (!o_ptr->is_melee_weapon()) {
            return false;
        }

        const auto &baseitem = o_ptr->get_baseitem();
        if ((o_ptr->dd == baseitem.dd) && (o_ptr->ds == baseitem.ds)) {
            return false;
        }

        if (!o_ptr->is_known() && object_is_quest_target(player_ptr->current_floor_ptr->quest_number, o_ptr)) {
            return false;
        }
    }

    if (entry.has(FLG_MORE_DICE)) {
        if (o_ptr->dd * o_ptr->ds < entry.dice) {
            return false;
        }
    }

    if (entry.has(FLG_MORE_BONUS)) {
        if (!o_ptr->is_known()) {
            return false;
        }

        if (o_ptr->pval) {
            if (o_ptr->pval < entry.bonus) {
                return false;
            }
        } else {
            if (o_ptr->to_h < entry.bonus && o_ptr->to_d < entry.bonus && o_ptr->to_a < entry.bonus && o_ptr->pval < entry.bonus) {
                return false;
            }
        }
    }

    if (entry.has(FLG_WORTHLESS) && (o_ptr->get_price() > 0)) {
        return false;
    }

    if (entry.has(FLG_ARTIFACT)) {
        if (!o_ptr->is_known() || !o_ptr->is_fixed_or_random_artifact()) {
            return false;
        }
    }

    if (entry.has(FLG_EGO)) {
        if (!o_ptr->is_ego()) {
            return false;
        }
        if (!o_ptr->is_known() && !((o_ptr->ident & IDENT_SENSE) && o_ptr->feeling == FEEL_EXCELLENT)) {
            return false;
        }
    }

    if (entry.has(FLG_GOOD)) {
        if (!o_ptr->is_equipment()) {
            return false;
        }
        if (o_ptr->is_known()) {
            if (!o_ptr->is_nameless()) {
                return false;
            }

            if (o_ptr->to_a <= 0 && (o_ptr->to_h + o_ptr->to_d) <= 0) {
                return false;
            }
        } else if (o_ptr->ident & IDENT_SENSE) {
            switch (o_ptr->feeling) {
            case FEEL_GOOD:
                break;

            default:
                return false;
            }
        } else {
            return false;
        }
    }

    if (entry.has(FLG_NAMELESS)) {
        if (!o_ptr->is_equipment()) {
            return false;
        }
        if (o_ptr->is_known()) {
            if (!o_ptr->is_nameless()) {
                return false;
            }
        } else if (o_ptr->ident & IDENT_SENSE) {
            switch (o_ptr->feeling) {
            case FEEL_AVERAGE:
            case FEEL_GOOD:
            case FEEL_BROKEN:
            case FEEL_CURSED:
                break;

            default:
                return false;
            }
        } else {
            return false;
        }
    }

    if (entry.has(FLG_AVERAGE)) {
        if (!o_ptr->is_equipment()) {
            return false;
        }
        if (o_ptr->is_known()) {
            if (!o_ptr->is_nameless()) {
                return false;
            }

            if (o_ptr->is_cursed() || o_ptr->is_broken()) {
                return false;
            }

            if (o_ptr->to_a > 0 || (o_ptr->to_h + o_ptr->to_d) > 0) {
                return false;
            }
        } else if (o_ptr->ident & IDENT_SENSE) {
            switch (o_ptr->feeling) {
            case FEEL_AVERAGE:
                break;

            default:
                return false;
            }
        } else {
            return false;
        }
    }

    if (entry.has(FLG_RARE) && !o_ptr->is_rare()) {
        return false;
    }

    if (entry.has(FLG_COMMON) && o_ptr->is_rare()) {
        return false;
    }

    if (entry.has(FLG_WANTED) && !object_is_bounty(player_ptr, o_ptr)) {
        return false;
    }

    // @details このタイミングでは、svalは絶対にnulloptにならない、はず.
    const auto &bi_key = o_ptr->bi_key;
    const auto tval = bi_key.tval();
    const auto sval = bi_key.sval().value();
    const auto r_idx = i2enum<MonsterRaceId>(o_ptr->pval);
    if (entry.has(FLG_UNIQUE) && ((tval != ItemKindType::CORPSE && tval != ItemKindType::STATUE) || monraces_info[r_idx].kind_flags.has_not(MonsterKindType::UNIQUE))) {
        return false;
    }

    if (entry.has(FLG_HUMAN) && (tval != ItemKindType::CORPSE || !angband_strchr("pht", monraces_info[r_idx].d_char))) {
        return false;
    }

    if (entry.has(FLG_UNREADABLE) && check_book_realm(player_ptr, bi_key)) {
        return false;
    }

    PlayerClass pc(player_ptr);
    auto realm_except_class = pc.equals(PlayerClassType::SORCERER) || pc.equals(PlayerClassType::RED_MAGE);

    if (entry.has(FLG_REALM1) && ((get_realm1_book(player_ptr) != tval) || realm_except_class)) {
        return false;
    }

    if (entry.has(FLG_REALM2) && ((get_realm2_book(player_ptr) != tval) || realm_except_class)) {
        return false;
    }

    if (entry.has(FLG_FIRST) && (!o_ptr->is_spell_book() || (sval != 0))) {
        return false;
    }

    if (entry.has(FLG_SECOND) && (!o_ptr->is_spell_book() || (sval != 1))) {
        return false;
    }

    if (entry.has(FLG_THIRD) && (!o_ptr->is_spell_book() || (sval != 2))) {
        return false;
    }

    if (entry.has(FLG_FOURTH) && (!o_ptr->is_spell_book() || (sval != 3))) {
        return false;
    }

    if (!check_item_features(player_ptr, entry, *o_ptr, tval)) {
        return false;
    }

    if (entry.name[0] == '^') {
        if (item_name == entry.name.substr(1, entry.name.length() - 1)) {
            return false;
        }
    } else {
        if (angband_strstr(item_name.data(), entry.name.data()) == nullptr) {
            return false;
        }
    }

    if (!entry.has(FLG_COLLECTING)) {
        return true;
    }

    for (int j = 0; j < INVEN_PACK; j++) {
        /*
         * 'Collecting' means the item must be absorbed
         * into an inventory slot.
         * But an item can not be absorbed into itself!
         */
        if ((&player_ptr->inventory_list[j] != o_ptr) && object_similar(&player_ptr->inventory_list[j], o_ptr)) {
            return true;
        }
    }

    return false;
}
