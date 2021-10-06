#include "permanent-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 突然変異による耐性フラグを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_mutation_flags(player_type *player_ptr, TrFlags &flags)
{
    if (player_ptr->muta.none())
        return;

    if (player_ptr->muta.has(MUTA::FLESH_ROT))
        flags.reset(TR_REGEN);
    if (player_ptr->muta.has_any_of({ MUTA::XTRA_FAT, MUTA::XTRA_LEGS, MUTA::SHORT_LEG }))
        flags.set(TR_SPEED);
    if (player_ptr->muta.has(MUTA::ELEC_TOUC))
        flags.set(TR_SH_ELEC);
    if (player_ptr->muta.has(MUTA::FIRE_BODY)) {
        flags.set(TR_SH_FIRE);
        flags.set(TR_LITE_1);
    }

    if (player_ptr->muta.has(MUTA::WINGS))
        flags.set(TR_LEVITATION);
    if (player_ptr->muta.has(MUTA::FEARLESS))
        flags.set(TR_RES_FEAR);
    if (player_ptr->muta.has(MUTA::REGEN))
        flags.set(TR_REGEN);
    if (player_ptr->muta.has(MUTA::ESP))
        flags.set(TR_TELEPATHY);
    if (player_ptr->muta.has(MUTA::MOTION))
        flags.set(TR_FREE_ACT);
}

/*!
 * @brief 性格による耐性フラグを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_personality_flags(player_type *player_ptr, TrFlags &flags)
{
    if (player_ptr->ppersonality == PERSONALITY_SEXY)
        flags.set(TR_AGGRAVATE);
    if (player_ptr->ppersonality == PERSONALITY_CHARGEMAN)
        flags.set(TR_RES_CONF);

    if (player_ptr->ppersonality != PERSONALITY_MUNCHKIN)
        return;

    flags.set(TR_RES_BLIND);
    flags.set(TR_RES_CONF);
    flags.set(TR_HOLD_EXP);
    if (player_ptr->pclass != PlayerClassType::NINJA)
        flags.set(TR_LITE_1);
    if (player_ptr->lev > 9)
        flags.set(TR_SPEED);
}

/*!
 * @brief プレイヤーの職業、種族に応じた耐性フラグを返す
 * Prints ratings on certain abilities
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @details
 * Obtain the "flags" for the player as if he was an item
 * @todo 最終的にplayer-status系列と統合する
 */
void player_flags(player_type *player_ptr, TrFlags &flags)
{
    flags.clear();

    flags.set(PlayerClass(player_ptr).tr_flags());
    flags.set(PlayerRace(player_ptr).tr_flags());

    add_mutation_flags(player_ptr, flags);
    add_personality_flags(player_ptr, flags);
}

void riding_flags(player_type *player_ptr, TrFlags &flags, TrFlags &negative_flags)
{
    flags.clear();
    negative_flags.clear();

    if (!player_ptr->riding)
        return;

    if (any_bits(has_levitation(player_ptr), FLAG_CAUSE_RIDING)) {
        flags.set(TR_LEVITATION);
    } else {
        negative_flags.set(TR_LEVITATION);
    }
}
