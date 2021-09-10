﻿#include "permanent-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "player-info/equipment-info.h"
#include "player/player-personality-types.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレーヤーの職業による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_class_flags(player_type *creature_ptr, TrFlags &flags)
{
    switch (creature_ptr->pclass) {
    case CLASS_WARRIOR: {
        if (creature_ptr->lev > 29)
            flags.set(TR_RES_FEAR);
        if (creature_ptr->lev > 44)
            flags.set(TR_REGEN);

        break;
    }
    case CLASS_SAMURAI: {
        if (creature_ptr->lev > 29)
            flags.set(TR_RES_FEAR);

        break;
    }
    case CLASS_PALADIN: {
        if (creature_ptr->lev > 39)
            flags.set(TR_RES_FEAR);

        break;
    }
    case CLASS_CHAOS_WARRIOR: {
        if (creature_ptr->lev > 29)
            flags.set(TR_RES_CHAOS);
        if (creature_ptr->lev > 39)
            flags.set(TR_RES_FEAR);

        break;
    }
    case CLASS_MONK:
    case CLASS_FORCETRAINER: {
        if ((creature_ptr->lev > 9) && !heavy_armor(creature_ptr))
            flags.set(TR_SPEED);
        if ((creature_ptr->lev > 24) && !heavy_armor(creature_ptr))
            flags.set(TR_FREE_ACT);

        break;
    }
    case CLASS_NINJA: {
        if (heavy_armor(creature_ptr)) {
            flags.set(TR_SPEED);
        } else {
            if ((!creature_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(creature_ptr))
                && (!creature_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(creature_ptr)))
                flags.set(TR_SPEED);
            if (creature_ptr->lev > 24 && !creature_ptr->is_icky_wield[0] && !creature_ptr->is_icky_wield[1])
                flags.set(TR_FREE_ACT);
        }

        flags.set(TR_SLOW_DIGEST);
        flags.set(TR_RES_FEAR);
        if (creature_ptr->lev > 19)
            flags.set(TR_RES_POIS);
        if (creature_ptr->lev > 24)
            flags.set(TR_SUST_DEX);
        if (creature_ptr->lev > 29)
            flags.set(TR_SEE_INVIS);

        break;
    }
    case CLASS_MINDCRAFTER: {
        if (creature_ptr->lev > 9)
            flags.set(TR_RES_FEAR);
        if (creature_ptr->lev > 19)
            flags.set(TR_SUST_WIS);
        if (creature_ptr->lev > 29)
            flags.set(TR_RES_CONF);
        if (creature_ptr->lev > 39)
            flags.set(TR_TELEPATHY);

        break;
    }
    case CLASS_BARD: {
        flags.set(TR_RES_SOUND);
        break;
    }
    case CLASS_BERSERKER: {
        flags.set(TR_SUST_STR);
        flags.set(TR_SUST_DEX);
        flags.set(TR_SUST_CON);
        flags.set(TR_REGEN);
        flags.set(TR_FREE_ACT);
        flags.set(TR_SPEED);
        if (creature_ptr->lev > 39)
            flags.set(TR_REFLECT);

        break;
    }
    case CLASS_MIRROR_MASTER: {
        if (creature_ptr->lev > 39)
            flags.set(TR_REFLECT);

        break;
    }
    case CLASS_ELEMENTALIST:
        if (has_element_resist(creature_ptr, ElementRealm::FIRE, 1))
            flags.set(TR_RES_FIRE);
        if (has_element_resist(creature_ptr, ElementRealm::ICE, 1))
            flags.set(TR_RES_COLD);
        if (has_element_resist(creature_ptr, ElementRealm::SKY, 1))
            flags.set(TR_RES_ELEC);
        if (has_element_resist(creature_ptr, ElementRealm::SEA, 1))
            flags.set(TR_RES_ACID);
        if (has_element_resist(creature_ptr, ElementRealm::DARKNESS, 1))
            flags.set(TR_RES_DARK);
        if (has_element_resist(creature_ptr, ElementRealm::DARKNESS, 30))
            flags.set(TR_RES_NETHER);
        if (has_element_resist(creature_ptr, ElementRealm::CHAOS, 1))
            flags.set(TR_RES_CONF);
        if (has_element_resist(creature_ptr, ElementRealm::CHAOS, 30))
            flags.set(TR_RES_CHAOS);
        if (has_element_resist(creature_ptr, ElementRealm::EARTH, 1))
            flags.set(TR_RES_SHARDS);
        if (has_element_resist(creature_ptr, ElementRealm::EARTH, 30))
            flags.set(TR_REFLECT);
        if (has_element_resist(creature_ptr, ElementRealm::DEATH, 1))
            flags.set(TR_RES_POIS);
        if (has_element_resist(creature_ptr, ElementRealm::DEATH, 30))
            flags.set(TR_RES_DISEN);
        break;
    default:
        break;
    }
}

/*!
 * @brief 突然変異による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_mutation_flags(player_type *creature_ptr, TrFlags &flags)
{
    if (creature_ptr->muta.none())
        return;

    if (creature_ptr->muta.has(MUTA::FLESH_ROT))
        flags.reset(TR_REGEN);
    if (creature_ptr->muta.has_any_of({ MUTA::XTRA_FAT, MUTA::XTRA_LEGS, MUTA::SHORT_LEG }))
        flags.set(TR_SPEED);
    if (creature_ptr->muta.has(MUTA::ELEC_TOUC))
        flags.set(TR_SH_ELEC);
    if (creature_ptr->muta.has(MUTA::FIRE_BODY)) {
        flags.set(TR_SH_FIRE);
        flags.set(TR_LITE_1);
    }

    if (creature_ptr->muta.has(MUTA::WINGS))
        flags.set(TR_LEVITATION);
    if (creature_ptr->muta.has(MUTA::FEARLESS))
        flags.set(TR_RES_FEAR);
    if (creature_ptr->muta.has(MUTA::REGEN))
        flags.set(TR_REGEN);
    if (creature_ptr->muta.has(MUTA::ESP))
        flags.set(TR_TELEPATHY);
    if (creature_ptr->muta.has(MUTA::MOTION))
        flags.set(TR_FREE_ACT);
}

/*!
 * @brief 性格による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_personality_flags(player_type *creature_ptr, TrFlags &flags)
{
    if (creature_ptr->pseikaku == PERSONALITY_SEXY)
        flags.set(TR_AGGRAVATE);
    if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN)
        flags.set(TR_RES_CONF);

    if (creature_ptr->pseikaku != PERSONALITY_MUNCHKIN)
        return;

    flags.set(TR_RES_BLIND);
    flags.set(TR_RES_CONF);
    flags.set(TR_HOLD_EXP);
    if (creature_ptr->pclass != CLASS_NINJA)
        flags.set(TR_LITE_1);
    if (creature_ptr->lev > 9)
        flags.set(TR_SPEED);
}

/*!
 * @brief 剣術家の型による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_kata_flags(player_type *creature_ptr, TrFlags &flags)
{
    if (creature_ptr->special_defense & KATA_FUUJIN)
        flags.set(TR_REFLECT);
    if (creature_ptr->special_defense & KAMAE_GENBU)
        flags.set(TR_REFLECT);
    if (creature_ptr->special_defense & KAMAE_SUZAKU)
        flags.set(TR_LEVITATION);
    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        flags.set(TR_RES_FIRE);
        flags.set(TR_RES_COLD);
        flags.set(TR_RES_ACID);
        flags.set(TR_RES_ELEC);
        flags.set(TR_RES_POIS);
        flags.set(TR_LEVITATION);
        flags.set(TR_SH_FIRE);
        flags.set(TR_SH_ELEC);
        flags.set(TR_SH_COLD);
    }

    if ((creature_ptr->special_defense & KATA_MUSOU) == 0)
        return;

    flags.set(TR_RES_FEAR);
    flags.set(TR_RES_LITE);
    flags.set(TR_RES_DARK);
    flags.set(TR_RES_BLIND);
    flags.set(TR_RES_CONF);
    flags.set(TR_RES_SOUND);
    flags.set(TR_RES_SHARDS);
    flags.set(TR_RES_NETHER);
    flags.set(TR_RES_NEXUS);
    flags.set(TR_RES_CHAOS);
    flags.set(TR_RES_DISEN);
    flags.set(TR_REFLECT);
    flags.set(TR_HOLD_EXP);
    flags.set(TR_FREE_ACT);
    flags.set(TR_SH_FIRE);
    flags.set(TR_SH_ELEC);
    flags.set(TR_SH_COLD);
    flags.set(TR_LEVITATION);
    flags.set(TR_LITE_1);
    flags.set(TR_SEE_INVIS);
    flags.set(TR_TELEPATHY);
    flags.set(TR_SLOW_DIGEST);
    flags.set(TR_REGEN);
    flags.set(TR_SUST_STR);
    flags.set(TR_SUST_INT);
    flags.set(TR_SUST_WIS);
    flags.set(TR_SUST_DEX);
    flags.set(TR_SUST_CON);
    flags.set(TR_SUST_CHR);
}

/*!
 * @brief プレイヤーの職業、種族に応じた耐性フラグを返す
 * Prints ratings on certain abilities
 * @param creature_ptr 参照元プレーヤーポインタ
 * @param flags フラグを保管する配列
 * @details
 * Obtain the "flags" for the player as if he was an item
 * @todo 最終的にplayer-status系列と統合する
 */
void player_flags(player_type *creature_ptr, TrFlags &flags)
{
    flags.clear();

    add_class_flags(creature_ptr, flags);
    add_player_race_flags(creature_ptr, flags);

    add_mutation_flags(creature_ptr, flags);
    add_personality_flags(creature_ptr, flags);
    add_kata_flags(creature_ptr, flags);
}

void riding_flags(player_type *creature_ptr, TrFlags &flags, TrFlags &negative_flags)
{
    flags.clear();
    negative_flags.clear();

    if (!creature_ptr->riding)
        return;

    if (any_bits(has_levitation(creature_ptr), FLAG_CAUSE_RIDING)) {
        flags.set(TR_LEVITATION);
    } else {
        negative_flags.set(TR_LEVITATION);
    }
}
