/*!
 * @file random-art-activation.cpp
 * @brief ランダムアーティファクトの発動実装定義
 */

#include "artifact/random-art-activation.h"
#include "artifact/random-art-bias-types.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/tr-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

static RandomArtActType invest_activation_elec(void)
{
    if (!one_in_(3)) {
        return RandomArtActType::BO_ELEC_1;
    }

    if (!one_in_(5)) {
        return RandomArtActType::BA_ELEC_2;
    }

    return RandomArtActType::BA_ELEC_3;
}

static RandomArtActType invest_activation_fire(void)
{
    if (!one_in_(3)) {
        return RandomArtActType::BO_FIRE_1;
    }

    if (!one_in_(5)) {
        return RandomArtActType::BA_FIRE_1;
    }

    return RandomArtActType::BA_FIRE_2;
}

static RandomArtActType invest_activation_cold(void)
{
    if (!one_in_(3)) {
        return RandomArtActType::BO_COLD_1;
    }

    if (!one_in_(3)) {
        return RandomArtActType::BA_COLD_1;
    }

    if (!one_in_(3)) {
        return RandomArtActType::BA_COLD_2;
    }

    return RandomArtActType::BA_COLD_3;
}

static RandomArtActType invest_activation_chaos(void)
{
    return one_in_(6) ? RandomArtActType::SUMMON_DEMON : RandomArtActType::CALL_CHAOS;
}

static RandomArtActType invest_activation_priest(void)
{
    if (one_in_(13)) {
        return RandomArtActType::CHARM_UNDEAD;
    }

    if (one_in_(12)) {
        return RandomArtActType::BANISH_EVIL;
    }

    if (one_in_(11)) {
        return RandomArtActType::DISP_EVIL;
    }

    if (one_in_(10)) {
        return RandomArtActType::PROT_EVIL;
    }

    if (one_in_(9)) {
        return RandomArtActType::CURE_1000;
    }

    if (one_in_(8)) {
        return RandomArtActType::CURE_700;
    }

    if (one_in_(7)) {
        return RandomArtActType::REST_ALL;
    }

    if (one_in_(6)) {
        return RandomArtActType::REST_EXP;
    }

    return RandomArtActType::CURE_MW;
}

static RandomArtActType invest_activation_necromancy(void)
{
    if (one_in_(66)) {
        return RandomArtActType::WRAITH;
    }

    if (one_in_(13)) {
        return RandomArtActType::DISP_GOOD;
    }

    if (one_in_(9)) {
        return RandomArtActType::MASS_GENO;
    }

    if (one_in_(8)) {
        return RandomArtActType::GENOCIDE;
    }

    if (one_in_(13)) {
        return RandomArtActType::SUMMON_UNDEAD;
    }

    if (one_in_(9)) {
        return RandomArtActType::DRAIN_2;
    }

    if (one_in_(6)) {
        return RandomArtActType::CHARM_UNDEAD;
    }

    return RandomArtActType::DRAIN_1;
}

static RandomArtActType invest_activation_law(void)
{
    if (one_in_(8)) {
        return RandomArtActType::BANISH_EVIL;
    }

    if (one_in_(4)) {
        return RandomArtActType::DISP_EVIL;
    }

    return RandomArtActType::PROT_EVIL;
}

static RandomArtActType invest_activation_rogue(void)
{
    if (one_in_(50)) {
        return RandomArtActType::SPEED;
    }

    if (one_in_(4)) {
        return RandomArtActType::SLEEP;
    }

    if (one_in_(3)) {
        return RandomArtActType::DETECT_ALL;
    }

    if (one_in_(8)) {
        return RandomArtActType::ID_FULL;
    }

    return RandomArtActType::ID_PLAIN;
}

static RandomArtActType invest_activation_mage(void)
{
    if (one_in_(20)) {
        return RandomArtActType::SUMMON_ELEMENTAL;
    }

    if (one_in_(10)) {
        return RandomArtActType::SUMMON_PHANTOM;
    }

    if (one_in_(5)) {
        return RandomArtActType::RUNE_EXPLO;
    }

    return RandomArtActType::ESP;
}

static RandomArtActType invest_activation_warrior(void)
{
    return one_in_(100) ? RandomArtActType::INVULN : RandomArtActType::BERSERK;
}

static RandomArtActType invest_activation_ranger(void)
{
    if (one_in_(20)) {
        return RandomArtActType::CHARM_ANIMALS;
    }

    if (one_in_(7)) {
        return RandomArtActType::SUMMON_ANIMAL;
    }

    if (one_in_(6)) {
        return RandomArtActType::CHARM_ANIMAL;
    }

    if (one_in_(4)) {
        return RandomArtActType::RESIST_ALL;
    }

    if (one_in_(3)) {
        return RandomArtActType::SATIATE;
    }

    return RandomArtActType::CURE_POISON;
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにバイアスに依存した発動を与える。/ Add one activaton of randam artifact depend on bias.
 * @details バイアスが無い場合、一部のバイアスの確率によっては one_ability() に処理が移行する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void give_activation_power(ObjectType *o_ptr)
{
    RandomArtActType type = RandomArtActType::NONE;
    int chance = 0;
    switch (o_ptr->artifact_bias) {
    case BIAS_ELEC:
        type = invest_activation_elec();
        chance = 101;
        break;
    case BIAS_POIS:
        type = RandomArtActType::BA_POIS_1;
        chance = 101;
        break;
    case BIAS_FIRE:
        type = invest_activation_fire();
        chance = 101;
        break;
    case BIAS_COLD:
        type = invest_activation_cold();
        chance = 101;
        break;
    case BIAS_CHAOS:
        type = invest_activation_chaos();
        chance = 50;
        break;
    case BIAS_PRIESTLY:
        type = invest_activation_priest();
        chance = 101;
        break;
    case BIAS_NECROMANTIC:
        type = invest_activation_necromancy();
        chance = 101;
        break;
    case BIAS_LAW:
        type = invest_activation_law();
        chance = 101;
        break;
    case BIAS_ROGUE:
        type = invest_activation_rogue();
        chance = 101;
        break;
    case BIAS_MAGE:
        type = invest_activation_mage();
        chance = 66;
        break;
    case BIAS_WARRIOR:
        type = invest_activation_warrior();
        chance = 80;
        break;
    case BIAS_RANGER:
        type = invest_activation_ranger();
        chance = 101;
        break;
    }

    if ((type == RandomArtActType::NONE) || (randint1(100) >= chance)) {
        one_activation(o_ptr);
        return;
    }

    o_ptr->activation_id = type;
    o_ptr->art_flags.set(TR_ACTIVATE);
    o_ptr->timeout = 0;
}
