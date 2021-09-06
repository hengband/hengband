/*!
 * @file random-art-activation.cpp
 * @brief ランダムアーティファクトの発動実装定義
 */


#include "artifact/random-art-activation.h"
#include "artifact/random-art-effects.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/tr-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

static int invest_activation_elec(void)
{
    if (!one_in_(3))
        return ACT_BO_ELEC_1;

    if (!one_in_(5))
        return ACT_BA_ELEC_2;

    return ACT_BA_ELEC_3;
}

static int invest_activation_fire(void)
{
    if (!one_in_(3))
        return ACT_BO_FIRE_1;

    if (!one_in_(5))
        return ACT_BA_FIRE_1;

    return ACT_BA_FIRE_2;
}

static int invest_activation_cold(void)
{
    if (!one_in_(3))
        return ACT_BO_COLD_1;

    if (!one_in_(3))
        return ACT_BA_COLD_1;

    if (!one_in_(3))
        return ACT_BA_COLD_2;

    return ACT_BA_COLD_3;
}

static int invest_activation_chaos(void) { return one_in_(6) ? ACT_SUMMON_DEMON : ACT_CALL_CHAOS; }

static int invest_activation_priest(void)
{
    if (one_in_(13))
        return ACT_CHARM_UNDEAD;

    if (one_in_(12))
        return ACT_BANISH_EVIL;

    if (one_in_(11))
        return ACT_DISP_EVIL;

    if (one_in_(10))
        return ACT_PROT_EVIL;

    if (one_in_(9))
        return ACT_CURE_1000;

    if (one_in_(8))
        return ACT_CURE_700;

    if (one_in_(7))
        return ACT_REST_ALL;

    if (one_in_(6))
        return ACT_REST_EXP;

    return ACT_CURE_MW;
}

static int invest_activation_necromancy(void)
{
    if (one_in_(66))
        return ACT_WRAITH;

    if (one_in_(13))
        return ACT_DISP_GOOD;

    if (one_in_(9))
        return ACT_MASS_GENO;

    if (one_in_(8))
        return ACT_GENOCIDE;

    if (one_in_(13))
        return ACT_SUMMON_UNDEAD;

    if (one_in_(9))
        return ACT_DRAIN_2;

    if (one_in_(6))
        return ACT_CHARM_UNDEAD;

    return ACT_DRAIN_1;
}

static int invest_activation_law(void)
{
    if (one_in_(8))
        return ACT_BANISH_EVIL;

    if (one_in_(4))
        return ACT_DISP_EVIL;

    return ACT_PROT_EVIL;
}

static int invest_activation_rogue(void)
{
    if (one_in_(50))
        return ACT_SPEED;

    if (one_in_(4))
        return ACT_SLEEP;

    if (one_in_(3))
        return ACT_DETECT_ALL;

    if (one_in_(8))
        return ACT_ID_FULL;

    return ACT_ID_PLAIN;
}

static int invest_activation_mage(void)
{
    if (one_in_(20))
        return ACT_SUMMON_ELEMENTAL;

    if (one_in_(10))
        return ACT_SUMMON_PHANTOM;

    if (one_in_(5))
        return ACT_RUNE_EXPLO;

    return ACT_ESP;
}

static int invest_activation_warrior(void) { return one_in_(100) ? ACT_INVULN : ACT_BERSERK; }

static int invest_activation_ranger(void)
{
    if (one_in_(20))
        return ACT_CHARM_ANIMALS;

    if (one_in_(7))
        return ACT_SUMMON_ANIMAL;

    if (one_in_(6))
        return ACT_CHARM_ANIMAL;

    if (one_in_(4))
        return ACT_RESIST_ALL;

    if (one_in_(3))
        return ACT_SATIATE;

    return ACT_CURE_POISON;
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにバイアスに依存した発動を与える。/ Add one activaton of randam artifact depend on bias.
 * @details バイアスが無い場合、一部のバイアスの確率によっては one_ability() に処理が移行する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void give_activation_power(object_type *o_ptr)
{
    int type = 0;
    int chance = 0;
    switch (o_ptr->artifact_bias) {
    case BIAS_ELEC:
        type = invest_activation_elec();
        chance = 101;
        break;
    case BIAS_POIS:
        type = ACT_BA_POIS_1;
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

    if (!type || (randint1(100) >= chance)) {
        one_activation(o_ptr);
        return;
    }

    o_ptr->xtra2 = (byte)type;
    o_ptr->art_flags.set(TR_ACTIVATE);
    o_ptr->timeout = 0;
}
