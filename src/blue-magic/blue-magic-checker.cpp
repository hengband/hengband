/*!
 * @file blue-magic-checker.cpp
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
#include "monster-race/race-ability-mask.h"
#include "mspell/monster-power-table.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player/attack-defense-types.h"
#include "status/experience.h"
#include "system/angband.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 青魔法のラーニング判定と成功した場合のラーニング処理
 * @param monspell ラーニングを試みるモンスター攻撃のID
 */
void learn_spell(PlayerType *player_ptr, MonsterAbilityType monspell)
{
    if (player_ptr->action != ACTION_LEARN) {
        return;
    }

    auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
    if (!bluemage_data || bluemage_data->learnt_blue_magics.has(monspell)) {
        return;
    }

    const auto effects = player_ptr->effects();
    const auto is_confused = effects->confusion()->is_confused();
    const auto is_blind = effects->blindness()->is_blind();
    const auto is_stunned = effects->stun()->is_stunned();
    const auto is_hallucinated = effects->hallucination()->is_hallucinated();
    const auto is_paralyzed = effects->paralysis()->is_paralyzed();
    if (is_confused || is_blind || is_hallucinated || is_stunned || is_paralyzed) {
        return;
    }

    const auto &monster_power = monster_powers.at(monspell);
    if (randint1(player_ptr->lev + 70) > monster_power.level + 40) {
        bluemage_data->learnt_blue_magics.set(monspell);
        msg_format(_("%sを学習した！", "You have learned %s!"), monster_power.name);
        gain_exp(player_ptr, monster_power.level * monster_power.smana);
        sound(SOUND_STUDY);
        bluemage_data->new_magic_learned = true;
        player_ptr->redraw |= PR_STATE;
    }
}

/*!
 * @brief モンスター特殊能力のフラグ配列から特定のタイプの能力だけを抜き出す処理
 * Extract monster spells mask for the given mode
 * @param ability_flags モンスター特殊能力のフラグ集合
 * @param type 抜き出したいタイプ
 */
void set_rf_masks(EnumClassFlagGroup<MonsterAbilityType> &ability_flags, BlueMagicType type)
{
    ability_flags.clear();

    switch (type) {
    case BlueMagicType::BOLT:
        ability_flags.set(RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK).reset(MonsterAbilityType::ROCKET);
        break;

    case BlueMagicType::BALL:
        ability_flags.set(RF_ABILITY_BALL_MASK).reset(RF_ABILITY_BREATH_MASK);
        break;

    case BlueMagicType::BREATH:
        ability_flags.set(RF_ABILITY_BREATH_MASK);
        break;

    case BlueMagicType::SUMMON:
        ability_flags.set(RF_ABILITY_SUMMON_MASK);
        break;

    case BlueMagicType::OTHER:
        ability_flags.set(RF_ABILITY_ATTACK_MASK);
        ability_flags.reset(RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK | RF_ABILITY_BALL_MASK | RF_ABILITY_INDIRECT_MASK);
        break;
    }
}
