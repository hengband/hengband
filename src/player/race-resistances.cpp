#include "player/race-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/samurai-data-type.h"
#include "player/special-defense-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの職業/種族による免疫フラグを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 */
void player_immunity(PlayerType *player_ptr, TrFlags &flags)
{
    flags.clear();

    const auto p_flags = (PlayerRace(player_ptr).tr_flags() | PlayerClass(player_ptr).tr_flags());

    if (p_flags.has(TR_IM_ACID)) {
        flags.set(TR_RES_ACID);
    }
    if (p_flags.has(TR_IM_COLD)) {
        flags.set(TR_RES_COLD);
    }
    if (p_flags.has(TR_IM_ELEC)) {
        flags.set(TR_RES_ELEC);
    }
    if (p_flags.has(TR_IM_FIRE)) {
        flags.set(TR_RES_FIRE);
    }
    if (p_flags.has(TR_IM_DARK)) {
        flags.set(TR_RES_DARK);
    }

    if (PlayerRace(player_ptr).equals(PlayerRaceType::SPECTRE)) {
        flags.set(TR_RES_NETHER);
    }
}

/*!
 * @brief プレイヤーの一時的魔法効果による免疫フラグを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 */
void tim_player_immunity(PlayerType *player_ptr, TrFlags &flags)
{
    flags.clear();

    if (player_ptr->special_defense & DEFENSE_ACID) {
        flags.set(TR_RES_ACID);
    }
    if (player_ptr->special_defense & DEFENSE_ELEC) {
        flags.set(TR_RES_ELEC);
    }
    if (player_ptr->special_defense & DEFENSE_FIRE) {
        flags.set(TR_RES_FIRE);
    }
    if (player_ptr->special_defense & DEFENSE_COLD) {
        flags.set(TR_RES_COLD);
    }
    if (player_ptr->wraith_form) {
        flags.set(TR_RES_DARK);
    }
}

/*!
 * @brief プレイヤーの装備による免疫フラグを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 */
void known_obj_immunity(PlayerType *player_ptr, TrFlags &flags)
{
    flags.clear();

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        ObjectType *o_ptr;
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx) {
            continue;
        }

        auto o_flags = object_flags_known(o_ptr);
        if (o_flags.has(TR_IM_ACID)) {
            flags.set(TR_RES_ACID);
        }
        if (o_flags.has(TR_IM_ELEC)) {
            flags.set(TR_RES_ELEC);
        }
        if (o_flags.has(TR_IM_FIRE)) {
            flags.set(TR_RES_FIRE);
        }
        if (o_flags.has(TR_IM_COLD)) {
            flags.set(TR_RES_COLD);
        }
    }
}

/*!
 * @brief プレイヤーの種族による弱点フラグを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 */
void player_vulnerability_flags(PlayerType *player_ptr, TrFlags &flags)
{
    flags.clear();

    if (player_ptr->muta.has(PlayerMutationType::VULN_ELEM) || PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        flags.set(TR_RES_ACID);
        flags.set(TR_RES_ELEC);
        flags.set(TR_RES_FIRE);
        flags.set(TR_RES_COLD);
    }

    const auto p_flags = PlayerRace(player_ptr).tr_flags();

    if (p_flags.has(TR_VUL_ACID)) {
        flags.set(TR_RES_ACID);
    }
    if (p_flags.has(TR_VUL_COLD)) {
        flags.set(TR_RES_COLD);
    }
    if (p_flags.has(TR_VUL_ELEC)) {
        flags.set(TR_RES_ELEC);
    }
    if (p_flags.has(TR_VUL_FIRE)) {
        flags.set(TR_RES_FIRE);
    }
    if (p_flags.has(TR_VUL_LITE)) {
        flags.set(TR_RES_LITE);
    }
}
