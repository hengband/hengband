#include "spell/spells-diceroll.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-status-table.h"
#include "room/rooms-builder.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief モンスター魅了用セービングスロー共通部(汎用系)
 * @param pow 魅了パワー
 * @param m_ptr 対象モンスター
 * @return 魅了に抵抗したらTRUE
 */
bool common_saving_throw_charm(PlayerType *player_ptr, int pow, monster_type *m_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];

    if (player_ptr->current_floor_ptr->inside_arena) {
        return true;
    }

    /* Memorize a flag */
    if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_ALL);
        }
        return true;
    }

    if (r_ptr->flags3 & RF3_NO_CONF) {
        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_flags3 |= (RF3_NO_CONF);
        }
        return true;
    }

    if (r_ptr->flags1 & RF1_QUESTOR || m_ptr->mflag2.has(MonsterConstantFlagType::NOPET)) {
        return true;
    }

    pow += (adj_chr_chm[player_ptr->stat_index[A_CHR]] - 1);
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) {
        pow = pow * 2 / 3;
    }
    return (r_ptr->level > randint1((pow - 10) < 1 ? 1 : (pow - 10)) + 5);
}

/*!
 * @brief モンスター服従用セービングスロー共通部(部族依存系)
 * @param pow 服従パワー
 * @param m_ptr 対象モンスター
 * @return 服従に抵抗したらTRUE
 */
bool common_saving_throw_control(PlayerType *player_ptr, int pow, monster_type *m_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];

    if (player_ptr->current_floor_ptr->inside_arena) {
        return true;
    }

    /* Memorize a flag */
    if (r_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_ALL);
        }
        return true;
    }

    if (r_ptr->flags1 & RF1_QUESTOR || m_ptr->mflag2.has(MonsterConstantFlagType::NOPET)) {
        return true;
    }

    pow += adj_chr_chm[player_ptr->stat_index[A_CHR]] - 1;
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) {
        pow = pow * 2 / 3;
    }
    return (r_ptr->level > randint1((pow - 10) < 1 ? 1 : (pow - 10)) + 5);
}

/*!
 * @brief 一部ボルト魔法のビーム化確率を算出する / Prepare standard probability to become beam for fire_bolt_or_beam()
 * @return ビーム化確率(%)
 * @details
 * ハードコーティングによる実装が行われている。
 * メイジは(レベル)%、ハイメイジ、スペルマスターは(レベル)%、それ以外の職業は(レベル/2)%
 */
PERCENTAGE beam_chance(PlayerType *player_ptr)
{
    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::MAGE)) {
        return (PERCENTAGE)(player_ptr->lev);
    }
    if (pc.equals(PlayerClassType::HIGH_MAGE) || pc.equals(PlayerClassType::SORCERER)) {
        return (PERCENTAGE)(player_ptr->lev + 10);
    }

    return (PERCENTAGE)(player_ptr->lev / 2);
}
