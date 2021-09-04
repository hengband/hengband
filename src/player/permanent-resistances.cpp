#include "permanent-resistances.h"
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
            add_flag(flags, TR_RES_FEAR);
        if (creature_ptr->lev > 44)
            add_flag(flags, TR_REGEN);

        break;
    }
    case CLASS_SAMURAI: {
        if (creature_ptr->lev > 29)
            add_flag(flags, TR_RES_FEAR);

        break;
    }
    case CLASS_PALADIN: {
        if (creature_ptr->lev > 39)
            add_flag(flags, TR_RES_FEAR);

        break;
    }
    case CLASS_CHAOS_WARRIOR: {
        if (creature_ptr->lev > 29)
            add_flag(flags, TR_RES_CHAOS);
        if (creature_ptr->lev > 39)
            add_flag(flags, TR_RES_FEAR);

        break;
    }
    case CLASS_MONK:
    case CLASS_FORCETRAINER: {
        if ((creature_ptr->lev > 9) && !heavy_armor(creature_ptr))
            add_flag(flags, TR_SPEED);
        if ((creature_ptr->lev > 24) && !heavy_armor(creature_ptr))
            add_flag(flags, TR_FREE_ACT);

        break;
    }
    case CLASS_NINJA: {
        if (heavy_armor(creature_ptr)) {
            add_flag(flags, TR_SPEED);
        } else {
            if ((!creature_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(creature_ptr))
                && (!creature_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(creature_ptr)))
                add_flag(flags, TR_SPEED);
            if (creature_ptr->lev > 24 && !creature_ptr->is_icky_wield[0] && !creature_ptr->is_icky_wield[1])
                add_flag(flags, TR_FREE_ACT);
        }

        add_flag(flags, TR_SLOW_DIGEST);
        add_flag(flags, TR_RES_FEAR);
        if (creature_ptr->lev > 19)
            add_flag(flags, TR_RES_POIS);
        if (creature_ptr->lev > 24)
            add_flag(flags, TR_SUST_DEX);
        if (creature_ptr->lev > 29)
            add_flag(flags, TR_SEE_INVIS);

        break;
    }
    case CLASS_MINDCRAFTER: {
        if (creature_ptr->lev > 9)
            add_flag(flags, TR_RES_FEAR);
        if (creature_ptr->lev > 19)
            add_flag(flags, TR_SUST_WIS);
        if (creature_ptr->lev > 29)
            add_flag(flags, TR_RES_CONF);
        if (creature_ptr->lev > 39)
            add_flag(flags, TR_TELEPATHY);

        break;
    }
    case CLASS_BARD: {
        add_flag(flags, TR_RES_SOUND);
        break;
    }
    case CLASS_BERSERKER: {
        add_flag(flags, TR_SUST_STR);
        add_flag(flags, TR_SUST_DEX);
        add_flag(flags, TR_SUST_CON);
        add_flag(flags, TR_REGEN);
        add_flag(flags, TR_FREE_ACT);
        add_flag(flags, TR_SPEED);
        if (creature_ptr->lev > 39)
            add_flag(flags, TR_REFLECT);

        break;
    }
    case CLASS_MIRROR_MASTER: {
        if (creature_ptr->lev > 39)
            add_flag(flags, TR_REFLECT);

        break;
    }
    case CLASS_ELEMENTALIST:
        if (has_element_resist(creature_ptr, ElementRealm::FIRE, 1))
            add_flag(flags, TR_RES_FIRE);
        if (has_element_resist(creature_ptr, ElementRealm::ICE, 1))
            add_flag(flags, TR_RES_COLD);
        if (has_element_resist(creature_ptr, ElementRealm::SKY, 1))
            add_flag(flags, TR_RES_ELEC);
        if (has_element_resist(creature_ptr, ElementRealm::SEA, 1))
            add_flag(flags, TR_RES_ACID);
        if (has_element_resist(creature_ptr, ElementRealm::DARKNESS, 1))
            add_flag(flags, TR_RES_DARK);
        if (has_element_resist(creature_ptr, ElementRealm::DARKNESS, 30))
            add_flag(flags, TR_RES_NETHER);
        if (has_element_resist(creature_ptr, ElementRealm::CHAOS, 1))
            add_flag(flags, TR_RES_CONF);
        if (has_element_resist(creature_ptr, ElementRealm::CHAOS, 30))
            add_flag(flags, TR_RES_CHAOS);
        if (has_element_resist(creature_ptr, ElementRealm::EARTH, 1))
            add_flag(flags, TR_RES_SHARDS);
        if (has_element_resist(creature_ptr, ElementRealm::EARTH, 30))
            add_flag(flags, TR_REFLECT);
        if (has_element_resist(creature_ptr, ElementRealm::DEATH, 1))
            add_flag(flags, TR_RES_POIS);
        if (has_element_resist(creature_ptr, ElementRealm::DEATH, 30))
            add_flag(flags, TR_RES_DISEN);
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
        remove_flag(flags, TR_REGEN);
    if (creature_ptr->muta.has_any_of({ MUTA::XTRA_FAT, MUTA::XTRA_LEGS, MUTA::SHORT_LEG }))
        add_flag(flags, TR_SPEED);
    if (creature_ptr->muta.has(MUTA::ELEC_TOUC))
        add_flag(flags, TR_SH_ELEC);
    if (creature_ptr->muta.has(MUTA::FIRE_BODY)) {
        add_flag(flags, TR_SH_FIRE);
        add_flag(flags, TR_LITE_1);
    }

    if (creature_ptr->muta.has(MUTA::WINGS))
        add_flag(flags, TR_LEVITATION);
    if (creature_ptr->muta.has(MUTA::FEARLESS))
        add_flag(flags, TR_RES_FEAR);
    if (creature_ptr->muta.has(MUTA::REGEN))
        add_flag(flags, TR_REGEN);
    if (creature_ptr->muta.has(MUTA::ESP))
        add_flag(flags, TR_TELEPATHY);
    if (creature_ptr->muta.has(MUTA::MOTION))
        add_flag(flags, TR_FREE_ACT);
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
        add_flag(flags, TR_AGGRAVATE);
    if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN)
        add_flag(flags, TR_RES_CONF);

    if (creature_ptr->pseikaku != PERSONALITY_MUNCHKIN)
        return;

    add_flag(flags, TR_RES_BLIND);
    add_flag(flags, TR_RES_CONF);
    add_flag(flags, TR_HOLD_EXP);
    if (creature_ptr->pclass != CLASS_NINJA)
        add_flag(flags, TR_LITE_1);
    if (creature_ptr->lev > 9)
        add_flag(flags, TR_SPEED);
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
        add_flag(flags, TR_REFLECT);
    if (creature_ptr->special_defense & KAMAE_GENBU)
        add_flag(flags, TR_REFLECT);
    if (creature_ptr->special_defense & KAMAE_SUZAKU)
        add_flag(flags, TR_LEVITATION);
    if (creature_ptr->special_defense & KAMAE_SEIRYU) {
        add_flag(flags, TR_RES_FIRE);
        add_flag(flags, TR_RES_COLD);
        add_flag(flags, TR_RES_ACID);
        add_flag(flags, TR_RES_ELEC);
        add_flag(flags, TR_RES_POIS);
        add_flag(flags, TR_LEVITATION);
        add_flag(flags, TR_SH_FIRE);
        add_flag(flags, TR_SH_ELEC);
        add_flag(flags, TR_SH_COLD);
    }

    if ((creature_ptr->special_defense & KATA_MUSOU) == 0)
        return;

    add_flag(flags, TR_RES_FEAR);
    add_flag(flags, TR_RES_LITE);
    add_flag(flags, TR_RES_DARK);
    add_flag(flags, TR_RES_BLIND);
    add_flag(flags, TR_RES_CONF);
    add_flag(flags, TR_RES_SOUND);
    add_flag(flags, TR_RES_SHARDS);
    add_flag(flags, TR_RES_NETHER);
    add_flag(flags, TR_RES_NEXUS);
    add_flag(flags, TR_RES_CHAOS);
    add_flag(flags, TR_RES_DISEN);
    add_flag(flags, TR_REFLECT);
    add_flag(flags, TR_HOLD_EXP);
    add_flag(flags, TR_FREE_ACT);
    add_flag(flags, TR_SH_FIRE);
    add_flag(flags, TR_SH_ELEC);
    add_flag(flags, TR_SH_COLD);
    add_flag(flags, TR_LEVITATION);
    add_flag(flags, TR_LITE_1);
    add_flag(flags, TR_SEE_INVIS);
    add_flag(flags, TR_TELEPATHY);
    add_flag(flags, TR_SLOW_DIGEST);
    add_flag(flags, TR_REGEN);
    add_flag(flags, TR_SUST_STR);
    add_flag(flags, TR_SUST_INT);
    add_flag(flags, TR_SUST_WIS);
    add_flag(flags, TR_SUST_DEX);
    add_flag(flags, TR_SUST_CON);
    add_flag(flags, TR_SUST_CHR);
}

/*!
 * @brief プレイヤーの職業、種族に応じた耐性フラグを返す
 * Prints ratings on certain abilities
 * @param creature_ptr 参照元クリーチャーポインタ
 * @param flags フラグを保管する配列
 * @details
 * Obtain the "flags" for the player as if he was an item
 * @todo 最終的にplayer-status系列と統合する
 */
void player_flags(player_type *creature_ptr, TrFlags &flags)
{
    flags.fill(0U);

    add_class_flags(creature_ptr, flags);
    add_player_race_flags(creature_ptr, flags);

    add_mutation_flags(creature_ptr, flags);
    add_personality_flags(creature_ptr, flags);
    add_kata_flags(creature_ptr, flags);
}

void riding_flags(player_type *creature_ptr, TrFlags &flags, TrFlags &negative_flags)
{
    flags.fill(0U);
    negative_flags.fill(0U);

    if (!creature_ptr->riding)
        return;

    if (any_bits(has_levitation(creature_ptr), FLAG_CAUSE_RIDING)) {
        add_flag(flags, TR_LEVITATION);
    } else {
        add_flag(negative_flags, TR_LEVITATION);
    }
}
