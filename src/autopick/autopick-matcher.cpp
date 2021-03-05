﻿/*!
 * todo 300行以上の凶悪関数なので後で分割しておく
 * @brief 床のアイテムが自動拾いに一致するかどうかを調べる関数だけを格納したファイル
 * @date 2020/04/25
 * @author Hourier
 */

#include "autopick/autopick-matcher.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-key-flag-process.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-bow.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-quest.h"
#include "object-hook/hook-weapon.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/player-realm.h"
#include "system/floor-type-definition.h"
#include "util/string-processor.h"

/*
 * A function for Auto-picker/destroyer
 * Examine whether the object matches to the entry
 */
bool is_autopick_match(player_type *player_ptr, object_type *o_ptr, autopick_type *entry, concptr o_name)
{
    concptr ptr = entry->name;
    if (IS_FLG(FLG_UNAWARE) && object_is_aware(o_ptr))
        return FALSE;

    if (IS_FLG(FLG_UNIDENTIFIED) && (object_is_known(o_ptr) || (o_ptr->ident & IDENT_SENSE)))
        return FALSE;

    if (IS_FLG(FLG_IDENTIFIED) && !object_is_known(o_ptr))
        return FALSE;

    if (IS_FLG(FLG_STAR_IDENTIFIED) && (!object_is_known(o_ptr) || !object_is_fully_known(o_ptr)))
        return FALSE;

    if (IS_FLG(FLG_BOOSTED)) {
        object_kind *k_ptr = &k_info[o_ptr->k_idx];
        if (!object_is_melee_weapon(o_ptr))
            return FALSE;

        if ((o_ptr->dd == k_ptr->dd) && (o_ptr->ds == k_ptr->ds))
            return FALSE;

        if (!object_is_known(o_ptr) && object_is_quest_target(player_ptr->current_floor_ptr->inside_quest, o_ptr)) {
            return FALSE;
        }
    }

    if (IS_FLG(FLG_MORE_DICE)) {
        if (o_ptr->dd * o_ptr->ds < entry->dice)
            return FALSE;
    }

    if (IS_FLG(FLG_MORE_BONUS)) {
        if (!object_is_known(o_ptr))
            return FALSE;

        if (o_ptr->pval) {
            if (o_ptr->pval < entry->bonus)
                return FALSE;
        } else {
            if (o_ptr->to_h < entry->bonus && o_ptr->to_d < entry->bonus && o_ptr->to_a < entry->bonus && o_ptr->pval < entry->bonus)
                return FALSE;
        }
    }

    if (IS_FLG(FLG_WORTHLESS) && object_value(player_ptr, o_ptr) > 0)
        return FALSE;

    if (IS_FLG(FLG_ARTIFACT)) {
        if (!object_is_known(o_ptr) || !object_is_artifact(o_ptr))
            return FALSE;
    }

    if (IS_FLG(FLG_EGO)) {
        if (!object_is_ego(o_ptr))
            return FALSE;
        if (!object_is_known(o_ptr) && !((o_ptr->ident & IDENT_SENSE) && o_ptr->feeling == FEEL_EXCELLENT))
            return FALSE;
    }

    if (IS_FLG(FLG_GOOD)) {
        if (!object_is_equipment(o_ptr))
            return FALSE;
        if (object_is_known(o_ptr)) {
            if (!object_is_nameless(player_ptr, o_ptr))
                return FALSE;

            if (o_ptr->to_a <= 0 && (o_ptr->to_h + o_ptr->to_d) <= 0)
                return FALSE;
        } else if (o_ptr->ident & IDENT_SENSE) {
            switch (o_ptr->feeling) {
            case FEEL_GOOD:
                break;

            default:
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    if (IS_FLG(FLG_NAMELESS)) {
        if (!object_is_equipment(o_ptr))
            return FALSE;
        if (object_is_known(o_ptr)) {
            if (!object_is_nameless(player_ptr, o_ptr))
                return FALSE;
        } else if (o_ptr->ident & IDENT_SENSE) {
            switch (o_ptr->feeling) {
            case FEEL_AVERAGE:
            case FEEL_GOOD:
            case FEEL_BROKEN:
            case FEEL_CURSED:
                break;

            default:
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    if (IS_FLG(FLG_AVERAGE)) {
        if (!object_is_equipment(o_ptr))
            return FALSE;
        if (object_is_known(o_ptr)) {
            if (!object_is_nameless(player_ptr, o_ptr))
                return FALSE;

            if (object_is_cursed(o_ptr) || object_is_broken(o_ptr))
                return FALSE;

            if (o_ptr->to_a > 0 || (o_ptr->to_h + o_ptr->to_d) > 0)
                return FALSE;
        } else if (o_ptr->ident & IDENT_SENSE) {
            switch (o_ptr->feeling) {
            case FEEL_AVERAGE:
                break;

            default:
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    if (IS_FLG(FLG_RARE) && !object_is_rare(o_ptr))
        return FALSE;

    if (IS_FLG(FLG_COMMON) && object_is_rare(o_ptr))
        return FALSE;

    if (IS_FLG(FLG_WANTED) && !object_is_bounty(player_ptr, o_ptr))
        return FALSE;

    if (IS_FLG(FLG_UNIQUE) && ((o_ptr->tval != TV_CORPSE && o_ptr->tval != TV_STATUE) || !(r_info[o_ptr->pval].flags1 & RF1_UNIQUE)))
        return FALSE;

    if (IS_FLG(FLG_HUMAN) && (o_ptr->tval != TV_CORPSE || !angband_strchr("pht", r_info[o_ptr->pval].d_char)))
        return FALSE;

    if (IS_FLG(FLG_UNREADABLE) && (o_ptr->tval < TV_LIFE_BOOK || check_book_realm(player_ptr, o_ptr->tval, o_ptr->sval)))
        return FALSE;

    if (IS_FLG(FLG_REALM1) && (get_realm1_book(player_ptr) != o_ptr->tval || player_ptr->pclass == CLASS_SORCERER || player_ptr->pclass == CLASS_RED_MAGE))
        return FALSE;

    if (IS_FLG(FLG_REALM2) && (get_realm2_book(player_ptr) != o_ptr->tval || player_ptr->pclass == CLASS_SORCERER || player_ptr->pclass == CLASS_RED_MAGE))
        return FALSE;

    if (IS_FLG(FLG_FIRST) && (o_ptr->tval < TV_LIFE_BOOK || 0 != o_ptr->sval))
        return FALSE;

    if (IS_FLG(FLG_SECOND) && (o_ptr->tval < TV_LIFE_BOOK || 1 != o_ptr->sval))
        return FALSE;

    if (IS_FLG(FLG_THIRD) && (o_ptr->tval < TV_LIFE_BOOK || 2 != o_ptr->sval))
        return FALSE;

    if (IS_FLG(FLG_FOURTH) && (o_ptr->tval < TV_LIFE_BOOK || 3 != o_ptr->sval))
        return FALSE;

    if (IS_FLG(FLG_WEAPONS)) {
        if (!object_is_weapon(player_ptr, o_ptr))
            return FALSE;
    } else if (IS_FLG(FLG_FAVORITE_WEAPONS)) {
        if (!object_is_favorite(player_ptr, o_ptr))
            return FALSE;
    } else if (IS_FLG(FLG_ARMORS)) {
        if (!object_is_armour(player_ptr, o_ptr))
            return FALSE;
    } else if (IS_FLG(FLG_MISSILES)) {
        if (!object_is_ammo(o_ptr))
            return FALSE;
    } else if (IS_FLG(FLG_DEVICES)) {
        switch (o_ptr->tval) {
        case TV_SCROLL:
        case TV_STAFF:
        case TV_WAND:
        case TV_ROD:
            break;
        default:
            return FALSE;
        }
    } else if (IS_FLG(FLG_LIGHTS)) {
        if (!(o_ptr->tval == TV_LITE))
            return FALSE;
    } else if (IS_FLG(FLG_JUNKS)) {
        switch (o_ptr->tval) {
        case TV_SKELETON:
        case TV_BOTTLE:
        case TV_JUNK:
        case TV_STATUE:
            break;
        default:
            return FALSE;
        }
    } else if (IS_FLG(FLG_CORPSES)) {
        if (o_ptr->tval != TV_CORPSE && o_ptr->tval != TV_SKELETON)
            return FALSE;
    } else if (IS_FLG(FLG_SPELLBOOKS)) {
        if (!(o_ptr->tval >= TV_LIFE_BOOK))
            return FALSE;
    } else if (IS_FLG(FLG_HAFTED)) {
        if (!(o_ptr->tval == TV_HAFTED))
            return FALSE;
    } else if (IS_FLG(FLG_SHIELDS)) {
        if (!(o_ptr->tval == TV_SHIELD))
            return FALSE;
    } else if (IS_FLG(FLG_BOWS)) {
        if (!(o_ptr->tval == TV_BOW))
            return FALSE;
    } else if (IS_FLG(FLG_RINGS)) {
        if (!(o_ptr->tval == TV_RING))
            return FALSE;
    } else if (IS_FLG(FLG_AMULETS)) {
        if (!(o_ptr->tval == TV_AMULET))
            return FALSE;
    } else if (IS_FLG(FLG_SUITS)) {
        if (!(o_ptr->tval == TV_DRAG_ARMOR || o_ptr->tval == TV_HARD_ARMOR || o_ptr->tval == TV_SOFT_ARMOR))
            return FALSE;
    } else if (IS_FLG(FLG_CLOAKS)) {
        if (!(o_ptr->tval == TV_CLOAK))
            return FALSE;
    } else if (IS_FLG(FLG_HELMS)) {
        if (!(o_ptr->tval == TV_CROWN || o_ptr->tval == TV_HELM))
            return FALSE;
    } else if (IS_FLG(FLG_GLOVES)) {
        if (!(o_ptr->tval == TV_GLOVES))
            return FALSE;
    } else if (IS_FLG(FLG_BOOTS)) {
        if (!(o_ptr->tval == TV_BOOTS))
            return FALSE;
    }

    if (*ptr == '^') {
        ptr++;
        if (strncmp(o_name, ptr, strlen(ptr)))
            return FALSE;
    } else {
        if (!angband_strstr(o_name, ptr))
            return FALSE;
    }

    if (!IS_FLG(FLG_COLLECTING))
        return TRUE;

    for (int j = 0; j < INVEN_PACK; j++) {
        /*
         * 'Collecting' means the item must be absorbed
         * into an inventory slot.
         * But an item can not be absorbed into itself!
         */
        if ((&player_ptr->inventory_list[j] != o_ptr) && object_similar(&player_ptr->inventory_list[j], o_ptr))
            return TRUE;
    }

    return FALSE;
}
