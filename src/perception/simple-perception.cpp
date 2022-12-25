/*!
 * @brief 疑似鑑定処理
 * @date 2020/05/15
 * @author Hourier
 */

#include "perception/simple-perception.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-describer.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player/player-status-flags.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 擬似鑑定を実際に行い判定を反映する
 * @param slot 擬似鑑定を行うプレイヤーの所持リストID
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param heavy 重度の擬似鑑定を行うならばTRUE
 */
static void sense_inventory_aux(PlayerType *player_ptr, INVENTORY_IDX slot, bool heavy)
{
    auto *o_ptr = &player_ptr->inventory_list[slot];
    GAME_TEXT o_name[MAX_NLEN];
    if (o_ptr->ident & (IDENT_SENSE)) {
        return;
    }
    if (o_ptr->is_known()) {
        return;
    }

    item_feel_type feel = (heavy ? pseudo_value_check_heavy(o_ptr) : pseudo_value_check_light(o_ptr));
    if (!feel) {
        return;
    }

    if ((player_ptr->muta.has(PlayerMutationType::BAD_LUCK)) && !randint0(13)) {
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
            if (heavy) {
                feel = randint0(3) ? FEEL_GOOD : FEEL_AVERAGE;
            } else {
                feel = FEEL_UNCURSED;
            }
            break;
        }
        case FEEL_AVERAGE: {
            feel = randint0(2) ? FEEL_CURSED : FEEL_GOOD;
            break;
        }
        case FEEL_GOOD: {
            if (heavy) {
                feel = randint0(3) ? FEEL_CURSED : FEEL_AVERAGE;
            } else {
                feel = FEEL_CURSED;
            }
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

        default:
            break;
        }
    }

    if (disturb_minor) {
        disturb(player_ptr, false, false);
    }

    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    if (slot >= INVEN_MAIN_HAND) {
#ifdef JP
        msg_format("%s%s(%c)は%sという感じがする...", describe_use(player_ptr, slot), o_name, index_to_label(slot), game_inscriptions[feel]);
#else
        msg_format("You feel the %s (%c) you are %s %s %s...", o_name, index_to_label(slot), describe_use(player_ptr, slot),
            ((o_ptr->number == 1) ? "is" : "are"), game_inscriptions[feel]);
#endif

    } else {
#ifdef JP
        msg_format("ザックの中の%s(%c)は%sという感じがする...", o_name, index_to_label(slot), game_inscriptions[feel]);
#else
        msg_format("You feel the %s (%c) in your pack %s %s...", o_name, index_to_label(slot), ((o_ptr->number == 1) ? "is" : "are"), game_inscriptions[feel]);
#endif
    }

    o_ptr->ident |= (IDENT_SENSE);
    o_ptr->feeling = feel;

    autopick_alter_item(player_ptr, slot, destroy_feeling);
    player_ptr->update |= (PU_COMBINE | PU_REORDER);
    player_ptr->window_flags |= (PW_INVEN | PW_EQUIP);
}

/*!
 * @brief 1プレイヤーターン毎に武器、防具の擬似鑑定が行われるかを判定する。
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
void sense_inventory1(PlayerType *player_ptr)
{
    PLAYER_LEVEL plev = player_ptr->lev;
    bool heavy = false;
    ItemEntity *o_ptr;
    if (player_ptr->effects()->confusion()->is_confused()) {
        return;
    }

    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::ARCHER:
    case PlayerClassType::SAMURAI:
    case PlayerClassType::CAVALRY: {
        if (0 != randint0(9000L / (plev * plev + 40))) {
            return;
        }

        heavy = true;
        break;
    }
    case PlayerClassType::SMITH: {
        if (0 != randint0(6000L / (plev * plev + 50))) {
            return;
        }

        heavy = true;
        break;
    }
    case PlayerClassType::MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
    case PlayerClassType::MAGIC_EATER:
    case PlayerClassType::ELEMENTALIST: {
        if (0 != randint0(240000L / (plev + 5))) {
            return;
        }

        break;
    }
    case PlayerClassType::PRIEST:
    case PlayerClassType::BARD: {
        if (0 != randint0(10000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::ROGUE:
    case PlayerClassType::NINJA: {
        if (0 != randint0(20000L / (plev * plev + 40))) {
            return;
        }

        heavy = true;
        break;
    }
    case PlayerClassType::RANGER: {
        if (0 != randint0(95000L / (plev * plev + 40))) {
            return;
        }

        heavy = true;
        break;
    }
    case PlayerClassType::PALADIN:
    case PlayerClassType::SNIPER: {
        if (0 != randint0(77777L / (plev * plev + 40))) {
            return;
        }

        heavy = true;
        break;
    }
    case PlayerClassType::WARRIOR_MAGE:
    case PlayerClassType::RED_MAGE: {
        if (0 != randint0(75000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::IMITATOR:
    case PlayerClassType::BLUE_MAGE:
    case PlayerClassType::MIRROR_MASTER: {
        if (0 != randint0(55000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::CHAOS_WARRIOR: {
        if (0 != randint0(80000L / (plev * plev + 40))) {
            return;
        }

        heavy = true;
        break;
    }
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER: {
        if (0 != randint0(20000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::TOURIST: {
        if (0 != randint0(20000L / ((plev + 50) * (plev + 50)))) {
            return;
        }

        heavy = true;
        break;
    }
    case PlayerClassType::BEASTMASTER: {
        if (0 != randint0(65000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::BERSERKER: {
        heavy = true;
        break;
    }

    case PlayerClassType::MAX:
        break;
    }

    if (compare_virtue(player_ptr, V_KNOWLEDGE, 100, VIRTUE_LARGE)) {
        heavy = true;
    }

    for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];

        if (!o_ptr->bi_id) {
            continue;
        }

        auto okay = false;
        switch (o_ptr->bi_key.tval()) {
        case ItemKindType::SHOT:
        case ItemKindType::ARROW:
        case ItemKindType::BOLT:
        case ItemKindType::BOW:
        case ItemKindType::DIGGING:
        case ItemKindType::HAFTED:
        case ItemKindType::POLEARM:
        case ItemKindType::SWORD:
        case ItemKindType::BOOTS:
        case ItemKindType::GLOVES:
        case ItemKindType::HELM:
        case ItemKindType::CROWN:
        case ItemKindType::SHIELD:
        case ItemKindType::CLOAK:
        case ItemKindType::SOFT_ARMOR:
        case ItemKindType::HARD_ARMOR:
        case ItemKindType::DRAG_ARMOR:
        case ItemKindType::CARD:
            okay = true;
            break;
        default:
            break;
        }

        if (!okay) {
            continue;
        }

        if ((i < INVEN_MAIN_HAND) && (0 != randint0(5))) {
            continue;
        }

        if (has_good_luck(player_ptr) && !randint0(13)) {
            heavy = true;
        }

        sense_inventory_aux(player_ptr, i, heavy);
    }
}

/*!
 * @brief 1プレイヤーターン毎に武器、防具以外の擬似鑑定が行われるかを判定する。
 */
void sense_inventory2(PlayerType *player_ptr)
{
    PLAYER_LEVEL plev = player_ptr->lev;
    ItemEntity *o_ptr;

    if (player_ptr->effects()->confusion()->is_confused()) {
        return;
    }

    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::ARCHER:
    case PlayerClassType::SAMURAI:
    case PlayerClassType::CAVALRY:
    case PlayerClassType::BERSERKER:
    case PlayerClassType::SNIPER: {
        return;
    }
    case PlayerClassType::SMITH:
    case PlayerClassType::PALADIN:
    case PlayerClassType::CHAOS_WARRIOR:
    case PlayerClassType::IMITATOR:
    case PlayerClassType::BEASTMASTER:
    case PlayerClassType::NINJA: {
        if (0 != randint0(240000L / (plev + 5))) {
            return;
        }

        break;
    }
    case PlayerClassType::RANGER:
    case PlayerClassType::WARRIOR_MAGE:
    case PlayerClassType::RED_MAGE:
    case PlayerClassType::MONK: {
        if (0 != randint0(95000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::PRIEST:
    case PlayerClassType::BARD:
    case PlayerClassType::ROGUE:
    case PlayerClassType::FORCETRAINER:
    case PlayerClassType::MINDCRAFTER: {
        if (0 != randint0(20000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
    case PlayerClassType::MAGIC_EATER:
    case PlayerClassType::MIRROR_MASTER:
    case PlayerClassType::BLUE_MAGE:
    case PlayerClassType::ELEMENTALIST: {
        if (0 != randint0(9000L / (plev * plev + 40))) {
            return;
        }

        break;
    }
    case PlayerClassType::TOURIST: {
        if (0 != randint0(20000L / ((plev + 50) * (plev + 50)))) {
            return;
        }

        break;
    }

    case PlayerClassType::MAX:
        break;
    }

    for (INVENTORY_IDX i = 0; i < INVEN_TOTAL; i++) {
        bool okay = false;
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->bi_id) {
            continue;
        }

        switch (o_ptr->bi_key.tval()) {
        case ItemKindType::RING:
        case ItemKindType::AMULET:
        case ItemKindType::LITE:
        case ItemKindType::FIGURINE: {
            okay = true;
            break;
        }

        default:
            break;
        }

        if (!okay) {
            continue;
        }

        if ((i < INVEN_MAIN_HAND) && (0 != randint0(5))) {
            continue;
        }

        sense_inventory_aux(player_ptr, i, true);
    }
}

/*!
 * @brief 重度擬似鑑定の判断処理 / Return a "feeling" (or nullptr) about an item.  Method 1 (Heavy).
 * @param o_ptr 擬似鑑定を行うオブジェクトの参照ポインタ。
 * @return 擬似鑑定結果のIDを返す。
 */
item_feel_type pseudo_value_check_heavy(ItemEntity *o_ptr)
{
    if (o_ptr->is_artifact()) {
        if (o_ptr->is_cursed() || o_ptr->is_broken()) {
            return FEEL_TERRIBLE;
        }

        return FEEL_SPECIAL;
    }

    if (o_ptr->is_ego()) {
        if (o_ptr->is_cursed() || o_ptr->is_broken()) {
            return FEEL_WORTHLESS;
        }

        return FEEL_EXCELLENT;
    }

    if (o_ptr->is_cursed()) {
        return FEEL_CURSED;
    }

    if (o_ptr->is_broken()) {
        return FEEL_BROKEN;
    }

    const auto tval = o_ptr->bi_key.tval();
    if ((tval == ItemKindType::RING) || (tval == ItemKindType::AMULET)) {
        return FEEL_AVERAGE;
    }

    if (o_ptr->to_a > 0) {
        return FEEL_GOOD;
    }

    if (o_ptr->to_h + o_ptr->to_d > 0) {
        return FEEL_GOOD;
    }

    return FEEL_AVERAGE;
}

/*!
 * @brief 軽度擬似鑑定の判断処理 / Return a "feeling" (or nullptr) about an item.  Method 2 (Light).
 * @param o_ptr 擬似鑑定を行うオブジェクトの参照ポインタ。
 * @return 擬似鑑定結果のIDを返す。
 */
item_feel_type pseudo_value_check_light(ItemEntity *o_ptr)
{
    if (o_ptr->is_cursed()) {
        return FEEL_CURSED;
    }

    if (o_ptr->is_broken()) {
        return FEEL_BROKEN;
    }

    if (o_ptr->is_artifact()) {
        return FEEL_UNCURSED;
    }

    if (o_ptr->is_ego()) {
        return FEEL_UNCURSED;
    }

    if (o_ptr->to_a > 0) {
        return FEEL_UNCURSED;
    }

    if (o_ptr->to_h + o_ptr->to_d > 0) {
        return FEEL_UNCURSED;
    }

    return FEEL_NONE;
}
