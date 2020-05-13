#include "angband.h"
#include "inventory/simple-appraiser.h"
#include "object-flavor.h"
#include "avatar.h"
#include "object-hook.h"
#include "object/object-kind.h"
#include "player-move.h"
#include "player-inventory.h"
#include "autopick/autopick.h"

/*!
 * @brief 擬似鑑定を実際に行い判定を反映する
 * @param slot 擬似鑑定を行うプレイヤーの所持リストID
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param heavy 重度の擬似鑑定を行うならばTRUE
 * @return なし
 */
static void sense_inventory_aux(player_type* creature_ptr, INVENTORY_IDX slot, bool heavy)
{
    byte feel;
    object_type* o_ptr = &creature_ptr->inventory_list[slot];
    GAME_TEXT o_name[MAX_NLEN];
    if (o_ptr->ident & (IDENT_SENSE))
        return;
    if (object_is_known(o_ptr))
        return;

    feel = (heavy ? value_check_aux1(o_ptr) : value_check_aux2(o_ptr));
    if (!feel)
        return;

    if ((creature_ptr->muta3 & MUT3_BAD_LUCK) && !randint0(13)) {
        switch (feel) {
        case FEEL_TERRIBLE: {
            feel = FEEL_SPECIAL;
            break;
        }
        case FEEL_WORTHLESS: {
            feel = FEEL_EXCELLENT;
            break;
        }
        case FEEL_CURSED: {
            if (heavy)
                feel = randint0(3) ? FEEL_GOOD : FEEL_AVERAGE;
            else
                feel = FEEL_UNCURSED;
            break;
        }
        case FEEL_AVERAGE: {
            feel = randint0(2) ? FEEL_CURSED : FEEL_GOOD;
            break;
        }
        case FEEL_GOOD: {
            if (heavy)
                feel = randint0(3) ? FEEL_CURSED : FEEL_AVERAGE;
            else
                feel = FEEL_CURSED;
            break;
        }
        case FEEL_EXCELLENT: {
            feel = FEEL_WORTHLESS;
            break;
        }
        case FEEL_SPECIAL: {
            feel = FEEL_TERRIBLE;
            break;
        }
        }
    }

    if (disturb_minor)
        disturb(creature_ptr, FALSE, FALSE);

    object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (slot >= INVEN_RARM) {
#ifdef JP
        msg_format("%s%s(%c)は%sという感じがする...",
            describe_use(creature_ptr, slot), o_name, index_to_label(slot), game_inscriptions[feel]);
#else
        msg_format("You feel the %s (%c) you are %s %s %s...",
            o_name, index_to_label(slot), describe_use(creature_ptr, slot),
            ((o_ptr->number == 1) ? "is" : "are"),
            game_inscriptions[feel]);
#endif

    } else {
#ifdef JP
        msg_format("ザックの中の%s(%c)は%sという感じがする...",
            o_name, index_to_label(slot), game_inscriptions[feel]);
#else
        msg_format("You feel the %s (%c) in your pack %s %s...",
            o_name, index_to_label(slot),
            ((o_ptr->number == 1) ? "is" : "are"),
            game_inscriptions[feel]);
#endif
    }

    o_ptr->ident |= (IDENT_SENSE);
    o_ptr->feeling = feel;

    autopick_alter_item(creature_ptr, slot, destroy_feeling);
    creature_ptr->update |= (PU_COMBINE | PU_REORDER);
    creature_ptr->window |= (PW_INVEN | PW_EQUIP);
}

/*!
 * @brief 1プレイヤーターン毎に武器、防具の擬似鑑定が行われるかを判定する。
 * @return なし
 * @details
 * Sense the inventory\n
 *\n
 *   Class 0 = Warrior --> fast and heavy\n
 *   Class 1 = Mage    --> slow and light\n
 *   Class 2 = Priest  --> fast but light\n
 *   Class 3 = Rogue   --> okay and heavy\n
 *   Class 4 = Ranger  --> slow but heavy  (changed!)\n
 *   Class 5 = Paladin --> slow but heavy\n
 */
void sense_inventory1(player_type* creature_ptr)
{
    PLAYER_LEVEL plev = creature_ptr->lev;
    bool heavy = FALSE;
    object_type* o_ptr;
    if (creature_ptr->confused)
        return;

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
    case CLASS_ARCHER:
    case CLASS_SAMURAI:
    case CLASS_CAVALRY: {
        if (0 != randint0(9000L / (plev * plev + 40)))
            return;

        heavy = TRUE;
        break;
    }
    case CLASS_SMITH: {
        if (0 != randint0(6000L / (plev * plev + 50)))
            return;

        heavy = TRUE;
        break;
    }
    case CLASS_MAGE:
    case CLASS_HIGH_MAGE:
    case CLASS_SORCERER:
    case CLASS_MAGIC_EATER: {
        if (0 != randint0(240000L / (plev + 5)))
            return;

        break;
    }
    case CLASS_PRIEST:
    case CLASS_BARD: {
        if (0 != randint0(10000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_ROGUE:
    case CLASS_NINJA: {
        if (0 != randint0(20000L / (plev * plev + 40)))
            return;

        heavy = TRUE;
        break;
    }
    case CLASS_RANGER: {
        if (0 != randint0(95000L / (plev * plev + 40)))
            return;

        heavy = TRUE;
        break;
    }
    case CLASS_PALADIN:
    case CLASS_SNIPER: {
        if (0 != randint0(77777L / (plev * plev + 40)))
            return;

        heavy = TRUE;
        break;
    }
    case CLASS_WARRIOR_MAGE:
    case CLASS_RED_MAGE: {
        if (0 != randint0(75000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_MINDCRAFTER:
    case CLASS_IMITATOR:
    case CLASS_BLUE_MAGE:
    case CLASS_MIRROR_MASTER: {
        if (0 != randint0(55000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_CHAOS_WARRIOR: {
        if (0 != randint0(80000L / (plev * plev + 40)))
            return;

        heavy = TRUE;
        break;
    }
    case CLASS_MONK:
    case CLASS_FORCETRAINER: {
        if (0 != randint0(20000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_TOURIST: {
        if (0 != randint0(20000L / ((plev + 50) * (plev + 50))))
            return;

        heavy = TRUE;
        break;
    }
    case CLASS_BEASTMASTER: {
        if (0 != randint0(65000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_BERSERKER: {
        heavy = TRUE;
        break;
    }
    }

    if (compare_virtue(creature_ptr, V_KNOWLEDGE, 100, VIRTUE_LARGE))
        heavy = TRUE;

    for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++) {
        bool okay = FALSE;

        o_ptr = &creature_ptr->inventory_list[i];

        if (!o_ptr->k_idx)
            continue;

        switch (o_ptr->tval) {
        case TV_SHOT:
        case TV_ARROW:
        case TV_BOLT:
        case TV_BOW:
        case TV_DIGGING:
        case TV_HAFTED:
        case TV_POLEARM:
        case TV_SWORD:
        case TV_BOOTS:
        case TV_GLOVES:
        case TV_HELM:
        case TV_CROWN:
        case TV_SHIELD:
        case TV_CLOAK:
        case TV_SOFT_ARMOR:
        case TV_HARD_ARMOR:
        case TV_DRAG_ARMOR:
        case TV_CARD: {
            okay = TRUE;
            break;
        }
        }

        if (!okay)
            continue;
        if ((i < INVEN_RARM) && (0 != randint0(5)))
            continue;

        if ((creature_ptr->muta3 & MUT3_GOOD_LUCK) && !randint0(13)) {
            heavy = TRUE;
        }

        sense_inventory_aux(creature_ptr, i, heavy);
    }
}

/*!
 * @brief 1プレイヤーターン毎に武器、防具以外の擬似鑑定が行われるかを判定する。
 * @return なし
 */
void sense_inventory2(player_type* creature_ptr)
{
    PLAYER_LEVEL plev = creature_ptr->lev;
    object_type* o_ptr;

    if (creature_ptr->confused)
        return;

    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR:
    case CLASS_ARCHER:
    case CLASS_SAMURAI:
    case CLASS_CAVALRY:
    case CLASS_BERSERKER:
    case CLASS_SNIPER: {
        return;
    }
    case CLASS_SMITH:
    case CLASS_PALADIN:
    case CLASS_CHAOS_WARRIOR:
    case CLASS_IMITATOR:
    case CLASS_BEASTMASTER:
    case CLASS_NINJA: {
        if (0 != randint0(240000L / (plev + 5)))
            return;

        break;
    }
    case CLASS_RANGER:
    case CLASS_WARRIOR_MAGE:
    case CLASS_RED_MAGE:
    case CLASS_MONK: {
        if (0 != randint0(95000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_PRIEST:
    case CLASS_BARD:
    case CLASS_ROGUE:
    case CLASS_FORCETRAINER:
    case CLASS_MINDCRAFTER: {
        if (0 != randint0(20000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_MAGE:
    case CLASS_HIGH_MAGE:
    case CLASS_SORCERER:
    case CLASS_MAGIC_EATER:
    case CLASS_MIRROR_MASTER:
    case CLASS_BLUE_MAGE: {
        if (0 != randint0(9000L / (plev * plev + 40)))
            return;

        break;
    }
    case CLASS_TOURIST: {
        if (0 != randint0(20000L / ((plev + 50) * (plev + 50))))
            return;

        break;
    }
    }

    for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++) {
        bool okay = FALSE;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        switch (o_ptr->tval) {
        case TV_RING:
        case TV_AMULET:
        case TV_LITE:
        case TV_FIGURINE: {
            okay = TRUE;
            break;
        }
        }

        if (!okay)
            continue;
        if ((i < INVEN_RARM) && (0 != randint0(5)))
            continue;

        sense_inventory_aux(creature_ptr, i, TRUE);
    }
}
