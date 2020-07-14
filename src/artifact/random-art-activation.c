#include "artifact/random-art-activation.h"
#include "artifact/random-art-bias-types.h"
#include "art-definition/random-art-effects.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/tr-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにバイアスに依存した発動を与える。/ Add one activaton of randam artifact depend on bias.
 * @details バイアスが無い場合、一部のバイアスの確率によっては one_ability() に処理が移行する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void give_activation_power(object_type *o_ptr)
{
    int type = 0, chance = 0;

    switch (o_ptr->artifact_bias) {
    case BIAS_ELEC:
        if (!one_in_(3)) {
            type = ACT_BO_ELEC_1;
        } else if (!one_in_(5)) {
            type = ACT_BA_ELEC_2;
        } else {
            type = ACT_BA_ELEC_3;
        }

        chance = 101;
        break;

    case BIAS_POIS:
        type = ACT_BA_POIS_1;
        chance = 101;
        break;

    case BIAS_FIRE:
        if (!one_in_(3)) {
            type = ACT_BO_FIRE_1;
        } else if (!one_in_(5)) {
            type = ACT_BA_FIRE_1;
        } else {
            type = ACT_BA_FIRE_2;
        }

        chance = 101;
        break;

    case BIAS_COLD:
        chance = 101;
        if (!one_in_(3))
            type = ACT_BO_COLD_1;
        else if (!one_in_(3))
            type = ACT_BA_COLD_1;
        else if (!one_in_(3))
            type = ACT_BA_COLD_2;
        else
            type = ACT_BA_COLD_3;
        break;

    case BIAS_CHAOS:
        chance = 50;
        if (one_in_(6))
            type = ACT_SUMMON_DEMON;
        else
            type = ACT_CALL_CHAOS;
        break;

    case BIAS_PRIESTLY:
        chance = 101;

        if (one_in_(13))
            type = ACT_CHARM_UNDEAD;
        else if (one_in_(12))
            type = ACT_BANISH_EVIL;
        else if (one_in_(11))
            type = ACT_DISP_EVIL;
        else if (one_in_(10))
            type = ACT_PROT_EVIL;
        else if (one_in_(9))
            type = ACT_CURE_1000;
        else if (one_in_(8))
            type = ACT_CURE_700;
        else if (one_in_(7))
            type = ACT_REST_ALL;
        else if (one_in_(6))
            type = ACT_REST_EXP;
        else
            type = ACT_CURE_MW;
        break;

    case BIAS_NECROMANTIC:
        chance = 101;
        if (one_in_(66))
            type = ACT_WRAITH;
        else if (one_in_(13))
            type = ACT_DISP_GOOD;
        else if (one_in_(9))
            type = ACT_MASS_GENO;
        else if (one_in_(8))
            type = ACT_GENOCIDE;
        else if (one_in_(13))
            type = ACT_SUMMON_UNDEAD;
        else if (one_in_(9))
            type = ACT_DRAIN_2;
        else if (one_in_(6))
            type = ACT_CHARM_UNDEAD;
        else
            type = ACT_DRAIN_1;
        break;

    case BIAS_LAW:
        chance = 101;
        if (one_in_(8))
            type = ACT_BANISH_EVIL;
        else if (one_in_(4))
            type = ACT_DISP_EVIL;
        else
            type = ACT_PROT_EVIL;
        break;

    case BIAS_ROGUE:
        chance = 101;
        if (one_in_(50))
            type = ACT_SPEED;
        else if (one_in_(4))
            type = ACT_SLEEP;
        else if (one_in_(3))
            type = ACT_DETECT_ALL;
        else if (one_in_(8))
            type = ACT_ID_FULL;
        else
            type = ACT_ID_PLAIN;
        break;

    case BIAS_MAGE:
        chance = 66;
        if (one_in_(20))
            type = ACT_SUMMON_ELEMENTAL;
        else if (one_in_(10))
            type = ACT_SUMMON_PHANTOM;
        else if (one_in_(5))
            type = ACT_RUNE_EXPLO;
        else
            type = ACT_ESP;
        break;

    case BIAS_WARRIOR:
        chance = 80;
        if (one_in_(100))
            type = ACT_INVULN;
        else
            type = ACT_BERSERK;
        break;

    case BIAS_RANGER:
        chance = 101;
        if (one_in_(20))
            type = ACT_CHARM_ANIMALS;
        else if (one_in_(7))
            type = ACT_SUMMON_ANIMAL;
        else if (one_in_(6))
            type = ACT_CHARM_ANIMAL;
        else if (one_in_(4))
            type = ACT_RESIST_ALL;
        else if (one_in_(3))
            type = ACT_SATIATE;
        else
            type = ACT_CURE_POISON;
        break;
    }

    if (!type || (randint1(100) >= chance)) {
        one_activation(o_ptr);
        return;
    }

    /* A type was chosen... */
    o_ptr->xtra2 = (byte)type;
    add_flag(o_ptr->art_flags, TR_ACTIVATE);
    o_ptr->timeout = 0;
}
