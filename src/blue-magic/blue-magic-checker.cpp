/*!
 * @brief 青魔法の処理実装 / Blue magic
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "blue-magic/blue-magic-checker.h"
#include "core/player-redraw-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-blue-mage.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags4.h"
#include "mspell/monster-power-table.h"
#include "mspell/mspell-mask-definitions.h"
#include "mspell/mspell-type.h"
#include "player/attack-defense-types.h"
#include "status/experience.h"
#include "view/display-messages.h"

/*!
 * @brief モンスター特技がラーニング可能かどうかを返す
 * @param ms_type モンスター特技ID
 */
bool monster_spell_is_learnable(const int monspell)
{
    switch (monspell) {
    case MS_DISPEL:
    case MS_ROCKET:
    case MS_SHOOT:
    case MS_BR_ACID:
    case MS_BR_ELEC:
    case MS_BR_FIRE:
    case MS_BR_COLD:
    case MS_BR_POIS:
    case MS_BR_NETHER:
    case MS_BR_LITE:
    case MS_BR_DARK:
    case MS_BR_CONF:
    case MS_BR_SOUND:
    case MS_BR_CHAOS:
    case MS_BR_DISEN:
    case MS_BR_NEXUS:
    case MS_BR_TIME:
    case MS_BR_INERTIA:
    case MS_BR_GRAVITY:
    case MS_BR_SHARDS:
    case MS_BR_PLASMA:
    case MS_BR_FORCE:
    case MS_BR_MANA:
    case MS_BALL_NUKE:
    case MS_BR_NUKE:
    case MS_BALL_CHAOS:
    case MS_BR_DISI:
    case MS_BALL_ACID:
    case MS_BALL_ELEC:
    case MS_BALL_FIRE:
    case MS_BALL_COLD:
    case MS_BALL_POIS:
    case MS_BALL_NETHER:
    case MS_BALL_WATER:
    case MS_BALL_MANA:
    case MS_BALL_DARK:
    case MS_DRAIN_MANA:
    case MS_MIND_BLAST:
    case MS_BRAIN_SMASH:
    case MS_CAUSE_1:
    case MS_CAUSE_2:
    case MS_CAUSE_3:
    case MS_CAUSE_4:
    case MS_BOLT_ACID:
    case MS_BOLT_ELEC:
    case MS_BOLT_FIRE:
    case MS_BOLT_COLD:
    case MS_STARBURST:
    case MS_BOLT_NETHER:
    case MS_BOLT_WATER:
    case MS_BOLT_MANA:
    case MS_BOLT_PLASMA:
    case MS_BOLT_ICE:
    case MS_MAGIC_MISSILE:
    case MS_SCARE:
    case MS_BLIND:
    case MS_CONF:
    case MS_SLOW:
    case MS_SLEEP:
    case MS_HAND_DOOM:
    case MS_TELE_TO:
    case MS_TELE_AWAY:
    case MS_PSY_SPEAR:
    case MS_DARKNESS:
    case MS_MAKE_TRAP:
    case MS_FORGET:
    case MS_S_KIN:
    case MS_S_CYBER:
    case MS_S_MONSTER:
    case MS_S_MONSTERS:
    case MS_S_ANT:
    case MS_S_SPIDER:
    case MS_S_HOUND:
    case MS_S_HYDRA:
    case MS_S_ANGEL:
    case MS_S_DEMON:
    case MS_S_UNDEAD:
    case MS_S_DRAGON:
    case MS_S_HI_UNDEAD:
    case MS_S_HI_DRAGON:
    case MS_S_AMBERITE:
    case MS_S_UNIQUE:
        return TRUE;
    default:
        return FALSE;
    }
}

/*!
 * @brief 青魔法のラーニング判定と成功した場合のラーニング処理
 * @param monspell ラーニングを試みるモンスター攻撃のID
 * @return なし
 */
void learn_spell(player_type *learner_ptr, int monspell)
{
    if (learner_ptr->action != ACTION_LEARN)
        return;
    if (monspell < 0)
        return;
    if (learner_ptr->magic_num2[monspell])
        return;
    if (learner_ptr->confused || learner_ptr->blind || learner_ptr->image || learner_ptr->stun || learner_ptr->paralyzed)
        return;
    if (randint1(learner_ptr->lev + 70) > monster_powers[monspell].level + 40) {
        learner_ptr->magic_num2[monspell] = 1;
        msg_format(_("%sを学習した！", "You have learned %s!"), monster_powers[monspell].name);
        gain_exp(learner_ptr, monster_powers[monspell].level * monster_powers[monspell].smana);
        sound(SOUND_STUDY);
        learner_ptr->new_mane = TRUE;
        learner_ptr->redraw |= PR_STATE;
    }
}

/*!
 * todo f4, f5, f6を構造体にまとめ直す
 * @brief モンスター特殊能力のフラグ配列から特定条件の魔法だけを抜き出す処理
 * Extract monster spells mask for the given mode
 * @param f4 モンスター特殊能力の4番目のフラグ配列
 * @param f5 モンスター特殊能力の5番目のフラグ配列
 * @param f6 モンスター特殊能力の6番目のフラグ配列
 * @param mode 抜き出したい条件
 * @return なし
 */
/*
 */
void set_rf_masks(BIT_FLAGS *f4, BIT_FLAGS *f5, BIT_FLAGS *f6, blue_magic_type mode)
{
    switch (mode) {
    case MONSPELL_TYPE_BOLT:
        *f4 = ((RF4_BOLT_MASK | RF4_BEAM_MASK) & ~(RF4_ROCKET));
        *f5 = RF5_BOLT_MASK | RF5_BEAM_MASK;
        *f6 = RF6_BOLT_MASK | RF6_BEAM_MASK;
        break;

    case MONSPELL_TYPE_BALL:
        *f4 = (RF4_BALL_MASK & ~(RF4_BREATH_MASK));
        *f5 = (RF5_BALL_MASK & ~(RF5_BREATH_MASK));
        *f6 = (RF6_BALL_MASK & ~(RF6_BREATH_MASK));
        break;

    case MONSPELL_TYPE_BREATH:
        *f4 = (BIT_FLAGS)RF4_BREATH_MASK;
        *f5 = RF5_BREATH_MASK;
        *f6 = RF6_BREATH_MASK;
        break;

    case MONSPELL_TYPE_SUMMON:
        *f4 = RF4_SUMMON_MASK;
        *f5 = RF5_SUMMON_MASK;
        *f6 = (BIT_FLAGS)RF6_SUMMON_MASK;
        break;

    case MONSPELL_TYPE_OTHER:
        *f4 = RF4_ATTACK_MASK & ~(RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK | RF4_INDIRECT_MASK);
        *f5 = RF5_ATTACK_MASK & ~(RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_INDIRECT_MASK);
        *f6 = RF6_ATTACK_MASK & ~(RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK | RF6_INDIRECT_MASK);
        break;
    }
}
