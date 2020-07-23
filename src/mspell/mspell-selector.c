/*!
 * @brief モンスターが詠唱する魔法を選択する処理
 * @date 2020/07/23
 * @author Hourier
 * @details ba-no-ru/rupa-toの特殊処理はここで実施
 */

#include "mspell/mspell-selector.h"
#include "floor/geometry.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-status.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-judgement.h"
#include "system/monster-type-definition.h"
#include "system/floor-type-definition.h"
#include "world/world.h"

/*!
 * @brief ID値が正しいモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for hurting the player (directly).
 * @param spell 判定対象のID
 * @return 正しいIDならばTRUEを返す。
 */
static bool spell_attack(byte spell)
{
    /* All RF4 spells hurt (except for shriek and dispel) */
    if (spell < 128 && spell > 98)
        return TRUE;

    /* Various "ball" spells */
    if (spell >= 128 && spell <= 128 + 8)
        return TRUE;

    /* "Cause wounds" and "bolt" spells */
    if (spell >= 128 + 12 && spell < 128 + 27)
        return TRUE;

    /* Hand of Doom */
    if (spell == 160 + 1)
        return TRUE;

    /* Psycho-Spear */
    if (spell == 160 + 11)
        return TRUE;

    /* Doesn't hurt */
    return FALSE;
}

/*!
 * @brief ID値が退避目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for escaping.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_escape(byte spell)
{
    /* Blink or Teleport */
    if (spell == 160 + 4 || spell == 160 + 5)
        return TRUE;

    /* Teleport the player away */
    if (spell == 160 + 9 || spell == 160 + 10)
        return TRUE;

    /* Isn't good for escaping */
    return FALSE;
}

/*!
 * @brief ID値が妨害目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_annoy(byte spell)
{
    /* Shriek */
    if (spell == 96 + 0)
        return TRUE;

    /* Brain smash, et al (added curses) */
    if (spell >= 128 + 9 && spell <= 128 + 14)
        return TRUE;

    /* Scare, confuse, blind, slow, paralyze */
    if (spell >= 128 + 27 && spell <= 128 + 31)
        return TRUE;

    /* Teleport to */
    if (spell == 160 + 8)
        return TRUE;

    /* Teleport level */
    if (spell == 160 + 10)
        return TRUE;

    /* Darkness, make traps, cause amnesia */
    if (spell >= 160 + 12 && spell <= 160 + 14)
        return TRUE;

    /* Doesn't annoy */
    return FALSE;
}

/*!
 * @brief ID値が召喚型のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_summon(byte spell) { return spell >= 160 + 16; }

/*!
 * @brief ID値が死者復活処理かどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 死者復活の処理ならばTRUEを返す。
 */
static bool spell_raise(byte spell) { return spell == 160 + 15; }

/*!
 * @brief ID値が戦術的なモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good in a tactical situation.
 * @param spell 判定対象のID
 * @return 戦術的な魔法のIDならばTRUEを返す。
 */
static bool spell_tactic(byte spell) { return spell == 160 + 4; }

/*!
 * @brief ID値が無敵化するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell makes invulnerable.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_invulner(byte spell) { return spell == 160 + 3; }

/*!
 * @brief ID値が加速するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell hastes.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_haste(byte spell) { return spell == 160 + 0; }

/*!
 * @brief ID値が時間停止を行うモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell world.
 * @param spell 判定対象のID
 * @return 時間停止魔法のIDならばTRUEを返す。
 */
static bool spell_world(byte spell) { return spell == 160 + 6; }

/*!
 * @brief ID値が特別効果のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell special.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param spell 判定対象のID
 * @return 特別効果魔法のIDならばTRUEを返す。
 */
static bool spell_special(player_type *target_ptr, byte spell)
{
    if (target_ptr->phase_out)
        return FALSE;

    return spell == 160 + 7;
}

/*!
 * @brief ID値が光の剣のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell psycho-spear.
 * @param spell 判定対象のID
 * @return 光の剣のIDならばTRUEを返す。
 */
static bool spell_psy_spe(byte spell) { return spell == 160 + 11; }

/*!
 * @brief ID値が治癒魔法かどうかを返す /
 * Return TRUE if a spell is good for healing.
 * @param spell 判定対象のID
 * @return 治癒魔法のIDならばTRUEを返す。
 */
static bool spell_heal(byte spell) { return spell == 160 + 2; }

/*!
 * @brief ID値が魔力消去かどうかを返す /
 * Return TRUE if a spell is good for dispel.
 * @param spell 判定対象のID
 * @return 魔力消去のIDならばTRUEを返す。
 */
static bool spell_dispel(byte spell) { return spell == 96 + 2; }

/*!
 * todo 長過ぎる。切り分けが必要
 * @brief モンスターの魔法選択ルーチン
 * Have a monster choose a spell from a list of "useful" spells.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの構造体配列ID
 * @param spells 候補魔法IDをまとめた配列
 * @param num spellsの長さ
 * @return 選択したモンスター魔法のID
 * @details
 * Note that this list does NOT include spells that will just hit\n
 * other monsters, and the list is restricted when the monster is\n
 * "desperate".  Should that be the job of this function instead?\n
 *\n
 * Stupid monsters will just pick a spell randomly.  Smart monsters\n
 * will choose more "intelligently".\n
 *\n
 * Use the helper functions above to put spells into categories.\n
 *\n
 * This function may well be an efficiency bottleneck.\n
 */
int choose_attack_spell(player_type *target_ptr, msa_type *msa_ptr)
{
    byte escape[96], escape_num = 0;
    byte attack[96], attack_num = 0;
    byte summon[96], summon_num = 0;
    byte tactic[96], tactic_num = 0;
    byte annoy[96], annoy_num = 0;
    byte invul[96], invul_num = 0;
    byte haste[96], haste_num = 0;
    byte world[96], world_num = 0;
    byte special[96], special_num = 0;
    byte psy_spe[96], psy_spe_num = 0;
    byte raise[96], raise_num = 0;
    byte heal[96], heal_num = 0;
    byte dispel[96], dispel_num = 0;

    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[msa_ptr->m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->flags2 & RF2_STUPID)
        return (msa_ptr->mspells[randint0(msa_ptr->num)]);

    for (int i = 0; i < msa_ptr->num; i++) {
        if (spell_escape(msa_ptr->mspells[i]))
            escape[escape_num++] = msa_ptr->mspells[i];

        if (spell_attack(msa_ptr->mspells[i]))
            attack[attack_num++] = msa_ptr->mspells[i];

        if (spell_summon(msa_ptr->mspells[i]))
            summon[summon_num++] = msa_ptr->mspells[i];

        if (spell_tactic(msa_ptr->mspells[i]))
            tactic[tactic_num++] = msa_ptr->mspells[i];

        if (spell_annoy(msa_ptr->mspells[i]))
            annoy[annoy_num++] = msa_ptr->mspells[i];

        if (spell_invulner(msa_ptr->mspells[i]))
            invul[invul_num++] = msa_ptr->mspells[i];

        if (spell_haste(msa_ptr->mspells[i]))
            haste[haste_num++] = msa_ptr->mspells[i];

        if (spell_world(msa_ptr->mspells[i]))
            world[world_num++] = msa_ptr->mspells[i];

        if (spell_special(target_ptr, msa_ptr->mspells[i]))
            special[special_num++] = msa_ptr->mspells[i];

        if (spell_psy_spe(msa_ptr->mspells[i]))
            psy_spe[psy_spe_num++] = msa_ptr->mspells[i];

        if (spell_raise(msa_ptr->mspells[i]))
            raise[raise_num++] = msa_ptr->mspells[i];

        if (spell_heal(msa_ptr->mspells[i]))
            heal[heal_num++] = msa_ptr->mspells[i];

        if (spell_dispel(msa_ptr->mspells[i]))
            dispel[dispel_num++] = msa_ptr->mspells[i];
    }

    if (world_num && (randint0(100) < 15) && !current_world_ptr->timewalk_m_idx)
        return (world[randint0(world_num)]);

    if (special_num) {
        bool success = FALSE;
        switch (m_ptr->r_idx) {
        case MON_BANOR:
        case MON_LUPART:
            if ((m_ptr->hp < m_ptr->maxhp / 2) && r_info[MON_BANOR].max_num && r_info[MON_LUPART].max_num)
                success = TRUE;
            break;
        default:
            break;
        }

        if (success)
            return (special[randint0(special_num)]);
    }

    if (m_ptr->hp < m_ptr->maxhp / 3 && one_in_(2)) {
        if (heal_num)
            return (heal[randint0(heal_num)]);
    }

    if (((m_ptr->hp < m_ptr->maxhp / 3) || monster_fear_remaining(m_ptr)) && one_in_(2)) {
        if (escape_num)
            return (escape[randint0(escape_num)]);
    }

    if (special_num) {
        bool success = FALSE;
        switch (m_ptr->r_idx) {
        case MON_OHMU:
        case MON_BANOR:
        case MON_LUPART:
            break;
        case MON_BANORLUPART:
            if (randint0(100) < 70)
                success = TRUE;
            break;
        case MON_ROLENTO:
            if (randint0(100) < 40)
                success = TRUE;
            break;
        default:
            if (randint0(100) < 50)
                success = TRUE;
            break;
        }
        if (success)
            return (special[randint0(special_num)]);
    }

    if ((distance(target_ptr->y, target_ptr->x, m_ptr->fy, m_ptr->fx) < 4) && (attack_num || (r_ptr->a_ability_flags2 & RF6_TRAPS)) && (randint0(100) < 75)
        && !current_world_ptr->timewalk_m_idx) {
        if (tactic_num)
            return (tactic[randint0(tactic_num)]);
    }

    if (summon_num && (randint0(100) < 40))
        return (summon[randint0(summon_num)]);

    if (dispel_num && one_in_(2)) {
        if (dispel_check(target_ptr, msa_ptr->m_idx)) {
            return (dispel[randint0(dispel_num)]);
        }
    }

    if (raise_num && (randint0(100) < 40))
        return (raise[randint0(raise_num)]);

    if (is_invuln(target_ptr)) {
        if (psy_spe_num && (randint0(100) < 50)) {
            return (psy_spe[randint0(psy_spe_num)]);
        } else if (attack_num && (randint0(100) < 40)) {
            return (attack[randint0(attack_num)]);
        }
    } else if (attack_num && (randint0(100) < 85)) {
        return (attack[randint0(attack_num)]);
    }

    if (tactic_num && (randint0(100) < 50) && !current_world_ptr->timewalk_m_idx)
        return (tactic[randint0(tactic_num)]);

    if (invul_num && !m_ptr->mtimed[MTIMED_INVULNER] && (randint0(100) < 50))
        return (invul[randint0(invul_num)]);

    if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && (randint0(100) < 25)) {
        if (heal_num)
            return (heal[randint0(heal_num)]);
    }

    if (haste_num && (randint0(100) < 20) && !monster_fast_remaining(m_ptr))
        return (haste[randint0(haste_num)]);

    if (annoy_num && (randint0(100) < 80))
        return (annoy[randint0(annoy_num)]);

    return 0;
}
