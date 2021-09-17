/*!
 * @brief モンスターが詠唱する魔法を選択する処理
 * @date 2020/07/23
 * @author Hourier
 * @details ba-no-ru/rupa-toの特殊処理はここで実施
 */

#include "mspell/mspell-selector.h"
#include "floor/geometry.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-status.h"
#include "mspell/mspell-attack-util.h"
#include "mspell/mspell-judgement.h"
#include "player/player-status.h"
#include "system/monster-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "world/world.h"

/*!
 * @brief 指定したID値が指定した範囲内のIDかどうかを返す
 *
 * enum値に対して範囲で判定するのはあまり好ましくないが、歴史的経緯により仕方がない
 *
 * @param spell 判定対象のID
 * @param start 範囲の開始ID
 * @param end 範囲の終了ID(このIDも含む)
 * @return IDが start <= spell <= end なら true、そうでなければ false
 */
static bool spell_in_between(RF_ABILITY spell, RF_ABILITY start, RF_ABILITY end)
{
    auto spell_int = enum2i(spell);
    return enum2i(start) <= spell_int && spell_int <= enum2i(end);
}

/*!
 * @brief ID値が攻撃魔法のIDかどうかを返す /
 * Return TRUE if a spell is good for hurting the player (directly).
 * @param spell 判定対象のID
 * @return 正しいIDならばTRUEを返す。
 */
static bool spell_attack(RF_ABILITY spell)
{
    /* All RF4 spells hurt (except for shriek and dispel) */
    if (spell_in_between(spell, RF_ABILITY::ROCKET, RF_ABILITY::BR_DISI))
        return true;

    /* Various "ball" spells */
    if (spell_in_between(spell, RF_ABILITY::BA_ACID, RF_ABILITY::BA_DARK))
        return true;

    /* "Cause wounds" and "bolt" spells */
    if (spell_in_between(spell, RF_ABILITY::CAUSE_1, RF_ABILITY::MISSILE))
        return true;

    /* Hand of Doom */
    if (spell == RF_ABILITY::HAND_DOOM)
        return true;

    /* Psycho-Spear */
    if (spell == RF_ABILITY::PSY_SPEAR)
        return true;

    /* Doesn't hurt */
    return false;
}

/*!
 * @brief ID値が退避目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for escaping.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_escape(RF_ABILITY spell)
{
    /* Blink or Teleport */
    if (spell == RF_ABILITY::BLINK || spell == RF_ABILITY::TPORT)
        return true;

    /* Teleport the player away */
    if (spell == RF_ABILITY::TELE_AWAY || spell == RF_ABILITY::TELE_LEVEL)
        return true;

    /* Isn't good for escaping */
    return false;
}

/*!
 * @brief ID値が妨害目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_annoy(RF_ABILITY spell)
{
    /* Shriek */
    if (spell == RF_ABILITY::SHRIEK)
        return true;

    /* Brain smash, et al (added curses) */
    if (spell_in_between(spell, RF_ABILITY::DRAIN_MANA, RF_ABILITY::CAUSE_4))
        return true;

    /* Scare, confuse, blind, slow, paralyze */
    if (spell_in_between(spell, RF_ABILITY::SCARE, RF_ABILITY::HOLD))
        return true;

    /* Teleport to */
    if (spell == RF_ABILITY::TELE_TO)
        return true;

    /* Teleport level */
    if (spell == RF_ABILITY::TELE_LEVEL)
        return true;

    /* Darkness, make traps, cause amnesia */
    if (spell_in_between(spell, RF_ABILITY::TRAPS, RF_ABILITY::RAISE_DEAD))
        return true;

    /* Doesn't annoy */
    return false;
}

/*!
 * @brief ID値が召喚型のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_summon(RF_ABILITY spell)
{
    return spell_in_between(spell, RF_ABILITY::S_KIN, RF_ABILITY::S_UNIQUE);
}

/*!
 * @brief ID値が死者復活処理かどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 死者復活の処理ならばTRUEを返す。
 */
static bool spell_raise(RF_ABILITY spell)
{
    return spell == RF_ABILITY::RAISE_DEAD;
}

/*!
 * @brief ID値が戦術的なモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good in a tactical situation.
 * @param spell 判定対象のID
 * @return 戦術的な魔法のIDならばTRUEを返す。
 */
static bool spell_tactic(RF_ABILITY spell)
{
    return spell == RF_ABILITY::BLINK;
}

/*!
 * @brief ID値が無敵化するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell makes invulnerable.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_invulner(RF_ABILITY spell)
{
    return spell == RF_ABILITY::INVULNER;
}

/*!
 * @brief ID値が加速するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell hastes.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_haste(RF_ABILITY spell)
{
    return spell == RF_ABILITY::HASTE;
}

/*!
 * @brief ID値が時間停止を行うモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell world.
 * @param spell 判定対象のID
 * @return 時間停止魔法のIDならばTRUEを返す。
 */
static bool spell_world(RF_ABILITY spell)
{
    return spell == RF_ABILITY::WORLD;
}

/*!
 * @brief ID値が特別効果のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell special.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 判定対象のID
 * @return 特別効果魔法のIDならばTRUEを返す。
 */
static bool spell_special(player_type *player_ptr, RF_ABILITY spell)
{
    if (player_ptr->phase_out)
        return false;

    return spell == RF_ABILITY::SPECIAL;
}

/*!
 * @brief ID値が光の剣のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell psycho-spear.
 * @param spell 判定対象のID
 * @return 光の剣のIDならばTRUEを返す。
 */
static bool spell_psy_spe(RF_ABILITY spell)
{
    return spell == RF_ABILITY::PSY_SPEAR;
}

/*!
 * @brief ID値が治癒魔法かどうかを返す /
 * Return TRUE if a spell is good for healing.
 * @param spell 判定対象のID
 * @return 治癒魔法のIDならばTRUEを返す。
 */
static bool spell_heal(RF_ABILITY spell)
{
    return spell == RF_ABILITY::HEAL;
}

/*!
 * @brief ID値が魔力消去かどうかを返す /
 * Return TRUE if a spell is good for dispel.
 * @param spell 判定対象のID
 * @return 魔力消去のIDならばTRUEを返す。
 */
static bool spell_dispel(RF_ABILITY spell)
{
    return spell == RF_ABILITY::DISPEL;
}

/*!
 * @brief モンスターの魔法選択ルーチン
 * Have a monster choose a spell from a list of "useful" spells.
 * @param player_ptr プレイヤーへの参照ポインタ
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
 * @todo 長過ぎる。切り分けが必要
 */
RF_ABILITY choose_attack_spell(player_type *player_ptr, msa_type *msa_ptr)
{
    std::vector<RF_ABILITY> escape;
    std::vector<RF_ABILITY> attack;
    std::vector<RF_ABILITY> summon;
    std::vector<RF_ABILITY> tactic;
    std::vector<RF_ABILITY> annoy;
    std::vector<RF_ABILITY> invul;
    std::vector<RF_ABILITY> haste;
    std::vector<RF_ABILITY> world;
    std::vector<RF_ABILITY> special;
    std::vector<RF_ABILITY> psy_spe;
    std::vector<RF_ABILITY> raise;
    std::vector<RF_ABILITY> heal;
    std::vector<RF_ABILITY> dispel;

    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[msa_ptr->m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (r_ptr->flags2 & RF2_STUPID)
        return (msa_ptr->mspells[randint0(msa_ptr->mspells.size())]);

    for (size_t i = 0; i < msa_ptr->mspells.size(); i++) {
        if (spell_escape(msa_ptr->mspells[i]))
            escape.push_back(msa_ptr->mspells[i]);

        if (spell_attack(msa_ptr->mspells[i]))
            attack.push_back(msa_ptr->mspells[i]);

        if (spell_summon(msa_ptr->mspells[i]))
            summon.push_back(msa_ptr->mspells[i]);

        if (spell_tactic(msa_ptr->mspells[i]))
            tactic.push_back(msa_ptr->mspells[i]);

        if (spell_annoy(msa_ptr->mspells[i]))
            annoy.push_back(msa_ptr->mspells[i]);

        if (spell_invulner(msa_ptr->mspells[i]))
            invul.push_back(msa_ptr->mspells[i]);

        if (spell_haste(msa_ptr->mspells[i]))
            haste.push_back(msa_ptr->mspells[i]);

        if (spell_world(msa_ptr->mspells[i]))
            world.push_back(msa_ptr->mspells[i]);

        if (spell_special(player_ptr, msa_ptr->mspells[i]))
            special.push_back(msa_ptr->mspells[i]);

        if (spell_psy_spe(msa_ptr->mspells[i]))
            psy_spe.push_back(msa_ptr->mspells[i]);

        if (spell_raise(msa_ptr->mspells[i]))
            raise.push_back(msa_ptr->mspells[i]);

        if (spell_heal(msa_ptr->mspells[i]))
            heal.push_back(msa_ptr->mspells[i]);

        if (spell_dispel(msa_ptr->mspells[i]))
            dispel.push_back(msa_ptr->mspells[i]);
    }

    if (!world.empty() && (randint0(100) < 15) && !w_ptr->timewalk_m_idx)
        return (world[randint0(world.size())]);

    if (!special.empty()) {
        bool success = false;
        switch (m_ptr->r_idx) {
        case MON_BANOR:
        case MON_LUPART:
            if ((m_ptr->hp < m_ptr->maxhp / 2) && r_info[MON_BANOR].max_num && r_info[MON_LUPART].max_num)
                success = true;
            break;
        default:
            break;
        }

        if (success)
            return (special[randint0(special.size())]);
    }

    if (m_ptr->hp < m_ptr->maxhp / 3 && one_in_(2)) {
        if (!heal.empty())
            return (heal[randint0(heal.size())]);
    }

    if (((m_ptr->hp < m_ptr->maxhp / 3) || monster_fear_remaining(m_ptr)) && one_in_(2)) {
        if (!escape.empty())
            return (escape[randint0(escape.size())]);
    }

    if (!special.empty()) {
        bool success = false;
        switch (m_ptr->r_idx) {
        case MON_OHMU:
        case MON_BANOR:
        case MON_LUPART:
            break;
        case MON_BANORLUPART:
            if (randint0(100) < 70)
                success = true;
            break;
        case MON_ROLENTO:
            if (randint0(100) < 40)
                success = true;
            break;
        default:
            if (randint0(100) < 50)
                success = true;
            break;
        }
        if (success)
            return (special[randint0(special.size())]);
    }

    if ((distance(player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx) < 4) && (!attack.empty() || r_ptr->ability_flags.has(RF_ABILITY::TRAPS)) && (randint0(100) < 75)
        && !w_ptr->timewalk_m_idx) {
        if (!tactic.empty())
            return (tactic[randint0(tactic.size())]);
    }

    if (!summon.empty() && (randint0(100) < 40))
        return (summon[randint0(summon.size())]);

    if (!dispel.empty() && one_in_(2)) {
        if (dispel_check(player_ptr, msa_ptr->m_idx)) {
            return (dispel[randint0(dispel.size())]);
        }
    }

    if (!raise.empty() && (randint0(100) < 40))
        return (raise[randint0(raise.size())]);

    if (is_invuln(player_ptr)) {
        if (!psy_spe.empty() && (randint0(100) < 50)) {
            return (psy_spe[randint0(psy_spe.size())]);
        } else if (!attack.empty() && (randint0(100) < 40)) {
            return (attack[randint0(attack.size())]);
        }
    } else if (!attack.empty() && (randint0(100) < 85)) {
        return (attack[randint0(attack.size())]);
    }

    if (!tactic.empty() && (randint0(100) < 50) && !w_ptr->timewalk_m_idx)
        return (tactic[randint0(tactic.size())]);

    if (!invul.empty() && !m_ptr->mtimed[MTIMED_INVULNER] && (randint0(100) < 50))
        return (invul[randint0(invul.size())]);

    if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && (randint0(100) < 25)) {
        if (!heal.empty())
            return (heal[randint0(heal.size())]);
    }

    if (!haste.empty() && (randint0(100) < 20) && !monster_fast_remaining(m_ptr))
        return (haste[randint0(haste.size())]);

    if (!annoy.empty() && (randint0(100) < 80))
        return (annoy[randint0(annoy.size())]);

    return RF_ABILITY::MAX;
}
