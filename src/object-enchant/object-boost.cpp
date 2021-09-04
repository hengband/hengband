#include "object-enchant/object-boost.h"
#include "artifact/random-art-effects.h"
#include "object-enchant/tr-types.h"
#include "object/object-kind.h"
#include "player-ability/player-ability-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 上質以上のオブジェクトに与えるための各種ボーナスを正規乱数も加えて算出する。
 * Help determine an "enchantment bonus" for an object.
 * @param max ボーナス値の限度
 * @param level ボーナス値に加味する基準生成階
 * @return 算出されたボーナス値
 * @details
 * To avoid floating point but still provide a smooth distribution of bonuses,\n
 * we simply round the results of ENERGY_DIVISION in such a way as to "average" the\n
 * correct floating point value.\n
 *\n
 * This function has been changed.  It uses "randnor()" to choose values from\n
 * a normal distribution, whose mean moves from zero towards the max as the\n
 * level increases, and whose standard deviation is equal to 1/4 of the max,\n
 * and whose values are forced to lie between zero and the max, inclusive.\n
 *\n
 * Since the "level" rarely passes 100 before Morgoth is dead, it is very\n
 * rare to get the "full" enchantment on an object, even a deep levels.\n
 *\n
 * It is always possible (albeit unlikely) to get the "full" enchantment.\n
 *\n
 * A sample distribution of values from "m_bonus(10, N)" is shown below:\n
 *\n
 *   N       0     1     2     3     4     5     6     7     8     9    10\n
 * ---    ----  ----  ----  ----  ----  ----  ----  ----  ----  ----  ----\n
 *   0   66.37 13.01  9.73  5.47  2.89  1.31  0.72  0.26  0.12  0.09  0.03\n
 *   8   46.85 24.66 12.13  8.13  4.20  2.30  1.05  0.36  0.19  0.08  0.05\n
 *  16   30.12 27.62 18.52 10.52  6.34  3.52  1.95  0.90  0.31  0.15  0.05\n
 *  24   22.44 15.62 30.14 12.92  8.55  5.30  2.39  1.63  0.62  0.28  0.11\n
 *  32   16.23 11.43 23.01 22.31 11.19  7.18  4.46  2.13  1.20  0.45  0.41\n
 *  40   10.76  8.91 12.80 29.51 16.00  9.69  5.90  3.43  1.47  0.88  0.65\n
 *  48    7.28  6.81 10.51 18.27 27.57 11.76  7.85  4.99  2.80  1.22  0.94\n
 *  56    4.41  4.73  8.52 11.96 24.94 19.78 11.06  7.18  3.68  1.96  1.78\n
 *  64    2.81  3.07  5.65  9.17 13.01 31.57 13.70  9.30  6.04  3.04  2.64\n
 *  72    1.87  1.99  3.68  7.15 10.56 20.24 25.78 12.17  7.52  4.42  4.62\n
 *  80    1.02  1.23  2.78  4.75  8.37 12.04 27.61 18.07 10.28  6.52  7.33\n
 *  88    0.70  0.57  1.56  3.12  6.34 10.06 15.76 30.46 12.58  8.47 10.38\n
 *  96    0.27  0.60  1.25  2.28  4.30  7.60 10.77 22.52 22.51 11.37 16.53\n
 * 104    0.22  0.42  0.77  1.36  2.62  5.33  8.93 13.05 29.54 15.23 22.53\n
 * 112    0.15  0.20  0.56  0.87  2.00  3.83  6.86 10.06 17.89 27.31 30.27\n
 * 120    0.03  0.11  0.31  0.46  1.31  2.48  4.60  7.78 11.67 25.53 45.72\n
 * 128    0.02  0.01  0.13  0.33  0.83  1.41  3.24  6.17  9.57 14.22 64.07\n
 */
int m_bonus(int max, DEPTH level)
{
    int bonus, stand, extra, value;

    /* Paranoia -- enforce maximal "level" */
    if (level > MAX_DEPTH - 1)
        level = MAX_DEPTH - 1;

    /* The "bonus" moves towards the max */
    bonus = ((max * level) / MAX_DEPTH);

    /* Hack -- determine fraction of error */
    extra = ((max * level) % MAX_DEPTH);

    /* Hack -- simulate floating point computations */
    if (randint0(MAX_DEPTH) < extra)
        bonus++;

    /* The "stand" is equal to one quarter of the max */
    stand = (max / 4);

    /* Hack -- determine fraction of error */
    extra = (max % 4);

    /* Hack -- simulate floating point computations */
    if (randint0(4) < extra)
        stand++;

    /* Choose an "interesting" value */
    value = randnor(bonus, stand);

    /* Enforce the minimum value */
    if (value < 0)
        return 0;

    /* Enforce the maximum value */
    if (value > max)
        return (max);
    return (value);
}

/*!
 * @brief 対象のオブジェクトにランダムな能力維持を一つ付加する。/ Choose one random sustain
 * @details 重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_sustain(object_type *o_ptr)
{
    switch (randint0(A_MAX)) {
    case 0:
        add_flag(o_ptr->art_flags, TR_SUST_STR);
        break;
    case 1:
        add_flag(o_ptr->art_flags, TR_SUST_INT);
        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_SUST_WIS);
        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_SUST_DEX);
        break;
    case 4:
        add_flag(o_ptr->art_flags, TR_SUST_CON);
        break;
    case 5:
        add_flag(o_ptr->art_flags, TR_SUST_CHR);
        break;
    }
}

/*!
 * @brief オブジェクトにランダムな強いESPを与える
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @return TR_ESP_NONLIVINGがついたならばTRUE
 */
bool add_esp_strong(object_type *o_ptr)
{
    bool nonliv = false;

    switch (randint1(3)) {
    case 1:
        add_flag(o_ptr->art_flags, TR_ESP_EVIL);
        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_TELEPATHY);
        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_ESP_NONLIVING);
        nonliv = true;
        break;
    }

    return nonliv;
}

/*!
 * @brief オブジェクトにランダムな弱いESPを与える
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param extra TRUEならばESPの最大付与数が増える(TRUE -> 3+1d6 / FALSE -> 1d3)
 */
void add_esp_weak(object_type *o_ptr, bool extra)
{
    int i;
    uint32_t weak_esp_list[] = {
        TR_ESP_ANIMAL,
        TR_ESP_UNDEAD,
        TR_ESP_DEMON,
        TR_ESP_ORC,
        TR_ESP_TROLL,
        TR_ESP_GIANT,
        TR_ESP_DRAGON,
        TR_ESP_HUMAN,
        TR_ESP_GOOD,
        TR_ESP_UNIQUE,
    };
    const int MAX_ESP_WEAK = sizeof(weak_esp_list) / sizeof(weak_esp_list[0]);
    const int add_count = MIN(MAX_ESP_WEAK, (extra) ? (3 + randint1(randint1(6))) : randint1(3));

    /* Add unduplicated weak esp flags randomly */
    for (i = 0; i < add_count; ++i) {
        int choice = rand_range(i, MAX_ESP_WEAK - 1);

        add_flag(o_ptr->art_flags, weak_esp_list[choice]);
        weak_esp_list[choice] = weak_esp_list[i];
    }
}

/*!
 * @brief 高級なテレパシー群を付ける
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @details
 * テレパシーの冠など。
 * ESPまたは邪ESPは1d3の種族ESPを得る。
 * 無ESPは3+1d6の種族ESPを得る。
 */
void add_high_telepathy(object_type* o_ptr)
{
    if (add_esp_strong(o_ptr))
        add_esp_weak(o_ptr, true);
    else
        add_esp_weak(o_ptr, false);
}

/*!
 * @brief テレパシー群を付ける
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @details
 * 鋭敏の帽子など。
 * ESP、邪ESP、無ESPまたは1d3の種族ESP。
 */
void add_low_telepathy(object_type *o_ptr)
{
    if (one_in_(2))
        add_esp_strong(o_ptr);
    else
        add_esp_weak(o_ptr, false);
}

/*!
 * @brief 対象のオブジェクトに元素耐性を一つ付加する。/ Choose one random element resistance
 * @details 候補は火炎、冷気、電撃、酸のいずれかであり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_ele_resistance(object_type *o_ptr)
{
    switch (randint0(4)) {
    case 0:
        add_flag(o_ptr->art_flags, TR_RES_ACID);
        break;
    case 1:
        add_flag(o_ptr->art_flags, TR_RES_ELEC);
        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_RES_COLD);
        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_RES_FIRE);
        break;
    }
}

/*!
 * @brief 対象のオブジェクトにドラゴン装備向け元素耐性を一つ付加する。/ Choose one random element or poison resistance
 * @details 候補は1/7の確率で毒、6/7の確率で火炎、冷気、電撃、酸のいずれか(one_ele_resistance()のコール)であり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_dragon_ele_resistance(object_type *o_ptr)
{
    if (one_in_(7)) {
        add_flag(o_ptr->art_flags, TR_RES_POIS);
    } else {
        one_ele_resistance(o_ptr);
    }
}

/*!
 * @brief 対象のオブジェクトにランダムな上位耐性を一つ付加する。/ Choose one random high resistance
 * @details 重複の抑止はない。候補は毒、閃光、暗黒、破片、盲目、混乱、地獄、因果混乱、カオス、劣化、恐怖、時間逆転、水、呪力のいずれか。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_high_resistance(object_type *o_ptr)
{
    switch (randint0(15)) {
    case 0:
        add_flag(o_ptr->art_flags, TR_RES_POIS);
        break;
    case 1:
        add_flag(o_ptr->art_flags, TR_RES_LITE);
        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_RES_DARK);
        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_RES_SHARDS);
        break;
    case 4:
        add_flag(o_ptr->art_flags, TR_RES_BLIND);
        break;
    case 5:
        add_flag(o_ptr->art_flags, TR_RES_CONF);
        break;
    case 6:
        add_flag(o_ptr->art_flags, TR_RES_SOUND);
        break;
    case 7:
        add_flag(o_ptr->art_flags, TR_RES_NETHER);
        break;
    case 8:
        add_flag(o_ptr->art_flags, TR_RES_NEXUS);
        break;
    case 9:
        add_flag(o_ptr->art_flags, TR_RES_CHAOS);
        break;
    case 10:
        add_flag(o_ptr->art_flags, TR_RES_DISEN);
        break;
    case 11:
        add_flag(o_ptr->art_flags, TR_RES_FEAR);
        break;
    case 12:
        add_flag(o_ptr->art_flags, TR_RES_TIME);
        break;
    case 13:
        add_flag(o_ptr->art_flags, TR_RES_WATER);
        break;
    case 14:
        add_flag(o_ptr->art_flags, TR_RES_CURSE);
        break;
    }
}

/*!
 * @brief ドラゴン装備にランダムな耐性を与える
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 */
void dragon_resist(object_type *o_ptr)
{
    do {
        if (one_in_(4))
            one_dragon_ele_resistance(o_ptr);
        else
            one_high_resistance(o_ptr);
    } while (one_in_(2));
}

/*!
 * @brief 対象のオブジェクトに耐性を一つ付加する。/ Choose one random resistance
 * @details 1/3で元素耐性(one_ele_resistance())、2/3で上位耐性(one_high_resistance)
 * をコールする。重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_resistance(object_type *o_ptr)
{
    if (one_in_(3)) {
        one_ele_resistance(o_ptr);
    } else {
        one_high_resistance(o_ptr);
    }
}

/*!
 * @brief 対象のオブジェクトに能力を一つ付加する。/ Choose one random ability
 * @details 候補は浮遊、永久光源+1、透明視、警告、遅消化、急回復、麻痺知らず、経験値維持のいずれか。
 * 重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_ability(object_type *o_ptr)
{
    switch (randint0(10)) {
    case 0:
        add_flag(o_ptr->art_flags, TR_LEVITATION);
        break;
    case 1:
        add_flag(o_ptr->art_flags, TR_LITE_1);
        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_SEE_INVIS);
        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_WARNING);
        break;
    case 4:
        add_flag(o_ptr->art_flags, TR_SLOW_DIGEST);
        break;
    case 5:
        add_flag(o_ptr->art_flags, TR_REGEN);
        break;
    case 6:
        add_flag(o_ptr->art_flags, TR_FREE_ACT);
        break;
    case 7:
        add_flag(o_ptr->art_flags, TR_HOLD_EXP);
        break;
    case 8:
    case 9:
        one_low_esp(o_ptr);
        break;
    }
}

/*!
 * @brief 対象のオブジェクトに弱いESPを一つ付加する。/ Choose one lower rank esp
 * @details 候補は動物、アンデッド、悪魔、オーク、トロル、巨人、
 * ドラゴン、人間、善良、ユニークESPのいずれかであり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_low_esp(object_type *o_ptr)
{
    switch (randint1(10)) {
    case 1:
        add_flag(o_ptr->art_flags, TR_ESP_ANIMAL);
        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_ESP_UNDEAD);
        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_ESP_DEMON);
        break;
    case 4:
        add_flag(o_ptr->art_flags, TR_ESP_ORC);
        break;
    case 5:
        add_flag(o_ptr->art_flags, TR_ESP_TROLL);
        break;
    case 6:
        add_flag(o_ptr->art_flags, TR_ESP_GIANT);
        break;
    case 7:
        add_flag(o_ptr->art_flags, TR_ESP_DRAGON);
        break;
    case 8:
        add_flag(o_ptr->art_flags, TR_ESP_HUMAN);
        break;
    case 9:
        add_flag(o_ptr->art_flags, TR_ESP_GOOD);
        break;
    case 10:
        add_flag(o_ptr->art_flags, TR_ESP_UNIQUE);
        break;
    }
}

/*!
 * @brief 対象のオブジェクトに発動を一つ付加する。/ Choose one random activation
 * @details 候補多数。ランダムアーティファクトのバイアスには一切依存せず、
 * whileループによる構造で能力的に強力なものほど確率を落としている。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_activation(object_type *o_ptr)
{
    int type = 0;
    PERCENTAGE chance = 0;
    while (randint1(100) >= chance) {
        type = randint1(ACT_MAX);
        switch (type) {
        case ACT_SUNLIGHT:
        case ACT_BO_MISS_1:
        case ACT_BA_POIS_1:
        case ACT_BO_ELEC_1:
        case ACT_BO_ACID_1:
        case ACT_BO_COLD_1:
        case ACT_BO_FIRE_1:
        case ACT_CONFUSE:
        case ACT_SLEEP:
        case ACT_QUAKE:
        case ACT_CURE_LW:
        case ACT_CURE_MW:
        case ACT_CURE_POISON:
        case ACT_BERSERK:
        case ACT_LIGHT:
        case ACT_MAP_LIGHT:
        case ACT_DEST_DOOR:
        case ACT_STONE_MUD:
        case ACT_TELEPORT:
            chance = 101;
            break;
        case ACT_BA_COLD_1:
        case ACT_BA_FIRE_1:
        case ACT_HYPODYNAMIA_1:
        case ACT_TELE_AWAY:
        case ACT_ESP:
        case ACT_RESIST_ALL:
        case ACT_DETECT_ALL:
        case ACT_RECALL:
        case ACT_SATIATE:
        case ACT_RECHARGE:
            chance = 85;
            break;
        case ACT_TERROR:
        case ACT_PROT_EVIL:
        case ACT_ID_PLAIN:
            chance = 75;
            break;
        case ACT_HYPODYNAMIA_2:
        case ACT_DRAIN_1:
        case ACT_BO_MISS_2:
        case ACT_BA_FIRE_2:
        case ACT_REST_EXP:
            chance = 66;
            break;
        case ACT_BA_FIRE_3:
        case ACT_BA_COLD_3:
        case ACT_BA_ELEC_3:
        case ACT_WHIRLWIND:
        case ACT_DRAIN_2:
        case ACT_CHARM_ANIMAL:
            chance = 50;
            break;
        case ACT_SUMMON_ANIMAL:
        case ACT_ANIM_DEAD:
            chance = 40;
            break;
        case ACT_DISP_EVIL:
        case ACT_BA_MISS_3:
        case ACT_DISP_GOOD:
        case ACT_BANISH_EVIL:
        case ACT_GENOCIDE:
        case ACT_MASS_GENO:
        case ACT_CHARM_UNDEAD:
        case ACT_CHARM_OTHER:
        case ACT_SUMMON_PHANTOM:
        case ACT_REST_ALL:
        case ACT_RUNE_EXPLO:
            chance = 33;
            break;
        case ACT_CALL_CHAOS:
        case ACT_ROCKET:
        case ACT_CHARM_ANIMALS:
        case ACT_CHARM_OTHERS:
        case ACT_SUMMON_ELEMENTAL:
        case ACT_CURE_700:
        case ACT_SPEED:
        case ACT_ID_FULL:
        case ACT_RUNE_PROT:
            chance = 25;
            break;
        case ACT_CURE_1000:
        case ACT_XTRA_SPEED:
        case ACT_DETECT_XTRA:
        case ACT_DIM_DOOR:
            chance = 10;
            break;
        case ACT_TREE_CREATION:
        case ACT_SUMMON_DEMON:
        case ACT_SUMMON_UNDEAD:
        case ACT_WRAITH:
        case ACT_INVULN:
        case ACT_ALCHEMY:
            chance = 5;
            break;
        default:
            chance = 0;
        }
    }

    o_ptr->xtra2 = (byte)type;
    add_flag(o_ptr->art_flags, TR_ACTIVATE);
    o_ptr->timeout = 0;
}

/*!
 * @brief 対象のオブジェクトに王者の指輪向けの上位耐性を一つ付加する。/ Choose one random high resistance
 * @details 候補は閃光、暗黒、破片、盲目、混乱、地獄、因果混乱、カオス、恐怖、時間逆転、水、呪力であり
 * 王者の指輪にあらかじめついている耐性をone_high_resistance()から除外したものである。
 * ランダム付加そのものに重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void one_lordly_high_resistance(object_type *o_ptr)
{
    switch (randint0(13)) {
    case 0:
        add_flag(o_ptr->art_flags, TR_RES_LITE);
        break;
    case 1:
        add_flag(o_ptr->art_flags, TR_RES_DARK);
        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_RES_SHARDS);
        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_RES_BLIND);
        break;
    case 4:
        add_flag(o_ptr->art_flags, TR_RES_CONF);
        break;
    case 5:
        add_flag(o_ptr->art_flags, TR_RES_SOUND);
        break;
    case 6:
        add_flag(o_ptr->art_flags, TR_RES_NETHER);
        break;
    case 7:
        add_flag(o_ptr->art_flags, TR_RES_NEXUS);
        break;
    case 8:
        add_flag(o_ptr->art_flags, TR_RES_CHAOS);
        break;
    case 9:
        add_flag(o_ptr->art_flags, TR_RES_FEAR);
        break;
    case 10:
        add_flag(o_ptr->art_flags, TR_RES_TIME);
        break;
    case 11:
        add_flag(o_ptr->art_flags, TR_RES_WATER);
        break;
    case 12:
        add_flag(o_ptr->art_flags, TR_RES_CURSE);
        break;
    }
}

/*!
 * @brief オブジェクトの重量を軽くする
 * @param o_ptr オブジェクト情報への参照ポインタ
 */
void make_weight_ligten(object_type* o_ptr)
{
    o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
}

/*!
 * @brief オブジェクトの重量を重くする
 * @param o_ptr オブジェクト情報への参照ポインタ
 */
void make_weight_heavy(object_type *o_ptr)
{
    o_ptr->weight = (4 * k_info[o_ptr->k_idx].weight / 3);
}

/*!
 * @brief オブジェクトのベースACを増やす
 * @param o_ptr オブジェクト情報への参照ポインタ
 * @details
 * 1/4を加算。最低+5を保証。
 */
void add_xtra_ac(object_type *o_ptr)
{
    o_ptr->ac += MAX(5, o_ptr->ac / 4);
}
