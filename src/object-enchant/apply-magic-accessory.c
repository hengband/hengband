/*!
 * todo かなり長い。要分割
 * @brief 防具系のアイテムを強化して(恐らく床に)生成する処理
 * @date 2020/06/02
 * @author Hourier
 */

#include "object-enchant/apply-magic-accessory.h"
#include "artifact/random-art-generator.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-checker.h"
#include "object/object-kind.h"
#include "sv-definition/sv-amulet-types.h"
#include "sv-definition/sv-ring-types.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 装飾品系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "ring" or "amulet"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special "pval boost" code for ring of speed\n
 * Hack -- note that some items must be cursed (or blessed)\n
 */
void apply_magic_accessary(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
{
    switch (o_ptr->tval) {
    case TV_RING: {
        switch (o_ptr->sval) {
        case SV_RING_ATTACKS: {
            o_ptr->pval = (PARAMETER_VALUE)m_bonus(2, level);
            if (one_in_(15))
                o_ptr->pval++;
            if (o_ptr->pval < 1)
                o_ptr->pval = 1;

            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->pval = 0 - (o_ptr->pval);
            }

            break;
        }
        case SV_RING_SHOTS: {
            break;
        }
        case SV_RING_STR:
        case SV_RING_CON:
        case SV_RING_DEX: {
            o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->pval = 0 - (o_ptr->pval);
            }

            break;
        }
        case SV_RING_SPEED: {
            o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, level);
            while (randint0(100) < 50)
                o_ptr->pval++;

            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->pval = 0 - (o_ptr->pval);
                break;
            }

            break;
        }
        case SV_RING_LORDLY: {
            do {
                one_lordly_high_resistance(o_ptr);
            } while (one_in_(4));

            o_ptr->to_a = 10 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, level);
            break;
        }
        case SV_RING_WARNING: {
            if (one_in_(3))
                one_low_esp(o_ptr);
            break;
        }
        case SV_RING_SEARCHING: {
            o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->pval = 0 - (o_ptr->pval);
            }

            break;
        }
        case SV_RING_FLAMES:
        case SV_RING_ACID:
        case SV_RING_ICE:
        case SV_RING_ELEC: {
            o_ptr->to_a = 5 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, level);
            break;
        }
        case SV_RING_WEAKNESS:
        case SV_RING_STUPIDITY: {
            o_ptr->ident |= (IDENT_BROKEN);
            o_ptr->curse_flags |= TRC_CURSED;
            o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, level));
            if (power > 0)
                power = 0 - power;

            break;
        }
        case SV_RING_WOE: {
            o_ptr->ident |= (IDENT_BROKEN);
            o_ptr->curse_flags |= TRC_CURSED;
            o_ptr->to_a = 0 - (5 + (ARMOUR_CLASS)m_bonus(10, level));
            o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, level));
            if (power > 0)
                power = 0 - power;

            break;
        }
        case SV_RING_DAMAGE: {
            o_ptr->to_d = 1 + randint1(5) + (HIT_POINT)m_bonus(16, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->to_d = 0 - o_ptr->to_d;
            }

            break;
        }
        case SV_RING_ACCURACY: {
            o_ptr->to_h = 1 + randint1(5) + (HIT_PROB)m_bonus(16, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->to_h = 0 - o_ptr->to_h;
            }

            break;
        }
        case SV_RING_PROTECTION: {
            o_ptr->to_a = 5 + randint1(8) + (ARMOUR_CLASS)m_bonus(10, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->to_a = 0 - o_ptr->to_a;
            }

            break;
        }
        case SV_RING_SLAYING: {
            o_ptr->to_d = randint1(5) + (HIT_POINT)m_bonus(12, level);
            o_ptr->to_h = randint1(5) + (HIT_PROB)m_bonus(12, level);

            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->to_h = 0 - o_ptr->to_h;
                o_ptr->to_d = 0 - o_ptr->to_d;
            }

            break;
        }
        case SV_RING_MUSCLE: {
            o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(3, level);
            if (one_in_(4))
                o_ptr->pval++;

            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= TRC_CURSED;
                o_ptr->pval = 0 - o_ptr->pval;
            }

            break;
        }
        case SV_RING_AGGRAVATION: {
            o_ptr->ident |= (IDENT_BROKEN);
            o_ptr->curse_flags |= TRC_CURSED;
            if (power > 0)
                power = 0 - power;
            break;
        }
        }

        /* power > 2 is debug only */
        if ((one_in_(400) && (power > 0) && !object_is_cursed(o_ptr) && (level > 79)) || (power > 2)) {
            o_ptr->pval = MIN(o_ptr->pval, 4);
            become_random_artifact(owner_ptr, o_ptr, FALSE);
        } else if ((power == 2) && one_in_(2)) {
            while (!o_ptr->name2) {
                int tmp = m_bonus(10, level);
                object_kind *k_ptr = &k_info[o_ptr->k_idx];
                switch (randint1(28)) {
                case 1:
                case 2:
                    o_ptr->name2 = EGO_RING_THROW;
                    break;
                case 3:
                case 4:
                    if (has_flag(k_ptr->flags, TR_REGEN))
                        break;
                    o_ptr->name2 = EGO_RING_REGEN;
                    break;
                case 5:
                case 6:
                    if (has_flag(k_ptr->flags, TR_LITE_1))
                        break;
                    o_ptr->name2 = EGO_RING_LITE;
                    break;
                case 7:
                case 8:
                    if (has_flag(k_ptr->flags, TR_TELEPORT))
                        break;
                    o_ptr->name2 = EGO_RING_TELEPORT;
                    break;
                case 9:
                case 10:
                    if (o_ptr->to_h)
                        break;
                    o_ptr->name2 = EGO_RING_TO_H;
                    break;
                case 11:
                case 12:
                    if (o_ptr->to_d)
                        break;
                    o_ptr->name2 = EGO_RING_TO_D;
                    break;
                case 13:
                    if ((o_ptr->to_h) || (o_ptr->to_d))
                        break;
                    o_ptr->name2 = EGO_RING_SLAY;
                    break;
                case 14:
                    if ((has_flag(k_ptr->flags, TR_STR)) || o_ptr->to_h || o_ptr->to_d)
                        break;
                    o_ptr->name2 = EGO_RING_WIZARD;
                    break;
                case 15:
                    if (has_flag(k_ptr->flags, TR_ACTIVATE))
                        break;
                    o_ptr->name2 = EGO_RING_HERO;
                    break;
                case 16:
                    if (has_flag(k_ptr->flags, TR_ACTIVATE))
                        break;
                    if (tmp > 8)
                        o_ptr->name2 = EGO_RING_MANA_BALL;
                    else if (tmp > 4)
                        o_ptr->name2 = EGO_RING_MANA_BOLT;
                    else
                        o_ptr->name2 = EGO_RING_MAGIC_MIS;
                    break;
                case 17:
                    if (has_flag(k_ptr->flags, TR_ACTIVATE))
                        break;
                    if (!(has_flag(k_ptr->flags, TR_RES_FIRE))
                        && (has_flag(k_ptr->flags, TR_RES_COLD) || has_flag(k_ptr->flags, TR_RES_ELEC) || has_flag(k_ptr->flags, TR_RES_ACID)))
                        break;
                    if (tmp > 7)
                        o_ptr->name2 = EGO_RING_DRAGON_F;
                    else if (tmp > 3)
                        o_ptr->name2 = EGO_RING_FIRE_BALL;
                    else
                        o_ptr->name2 = EGO_RING_FIRE_BOLT;
                    break;
                case 18:
                    if (has_flag(k_ptr->flags, TR_ACTIVATE))
                        break;
                    if (!(has_flag(k_ptr->flags, TR_RES_COLD))
                        && (has_flag(k_ptr->flags, TR_RES_FIRE) || has_flag(k_ptr->flags, TR_RES_ELEC) || has_flag(k_ptr->flags, TR_RES_ACID)))
                        break;
                    if (tmp > 7)
                        o_ptr->name2 = EGO_RING_DRAGON_C;
                    else if (tmp > 3)
                        o_ptr->name2 = EGO_RING_COLD_BALL;
                    else
                        o_ptr->name2 = EGO_RING_COLD_BOLT;
                    break;
                case 19:
                    if (has_flag(k_ptr->flags, TR_ACTIVATE))
                        break;
                    if (!(has_flag(k_ptr->flags, TR_RES_ELEC))
                        && (has_flag(k_ptr->flags, TR_RES_COLD) || has_flag(k_ptr->flags, TR_RES_FIRE) || has_flag(k_ptr->flags, TR_RES_ACID)))
                        break;
                    if (tmp > 4)
                        o_ptr->name2 = EGO_RING_ELEC_BALL;
                    else
                        o_ptr->name2 = EGO_RING_ELEC_BOLT;
                    break;
                case 20:
                    if (has_flag(k_ptr->flags, TR_ACTIVATE))
                        break;
                    if (!(has_flag(k_ptr->flags, TR_RES_ACID))
                        && (has_flag(k_ptr->flags, TR_RES_COLD) || has_flag(k_ptr->flags, TR_RES_ELEC) || has_flag(k_ptr->flags, TR_RES_FIRE)))
                        break;
                    if (tmp > 4)
                        o_ptr->name2 = EGO_RING_ACID_BALL;
                    else
                        o_ptr->name2 = EGO_RING_ACID_BOLT;
                    break;
                case 21:
                case 22:
                case 23:
                case 24:
                case 25:
                case 26:
                    switch (o_ptr->sval) {
                    case SV_RING_SPEED:
                        if (!one_in_(3))
                            break;
                        o_ptr->name2 = EGO_RING_D_SPEED;
                        break;
                    case SV_RING_DAMAGE:
                    case SV_RING_ACCURACY:
                    case SV_RING_SLAYING:
                        if (one_in_(2))
                            break;
                        if (one_in_(2))
                            o_ptr->name2 = EGO_RING_HERO;
                        else {
                            o_ptr->name2 = EGO_RING_BERSERKER;
                            o_ptr->to_h -= 2 + randint1(4);
                            o_ptr->to_d += 2 + randint1(4);
                        }

                        break;
                    case SV_RING_PROTECTION:
                        o_ptr->name2 = EGO_RING_SUPER_AC;
                        o_ptr->to_a += 7 + m_bonus(5, level);
                        break;
                    case SV_RING_RES_FEAR:
                        o_ptr->name2 = EGO_RING_HERO;
                        break;
                    case SV_RING_SHOTS:
                        if (one_in_(2))
                            break;
                        o_ptr->name2 = EGO_RING_HUNTER;
                        break;
                    case SV_RING_SEARCHING:
                        o_ptr->name2 = EGO_RING_STEALTH;
                        break;
                    case SV_RING_TELEPORTATION:
                        o_ptr->name2 = EGO_RING_TELE_AWAY;
                        break;
                    case SV_RING_RES_BLINDNESS:
                        if (one_in_(2))
                            o_ptr->name2 = EGO_RING_RES_LITE;
                        else
                            o_ptr->name2 = EGO_RING_RES_DARK;
                        break;
                    case SV_RING_LORDLY:
                        if (!one_in_(20))
                            break;
                        one_lordly_high_resistance(o_ptr);
                        one_lordly_high_resistance(o_ptr);
                        o_ptr->name2 = EGO_RING_TRUE;
                        break;
                    case SV_RING_SUSTAIN:
                        if (!one_in_(4))
                            break;
                        o_ptr->name2 = EGO_RING_RES_TIME;
                        break;
                    case SV_RING_FLAMES:
                        if (!one_in_(2))
                            break;
                        o_ptr->name2 = EGO_RING_DRAGON_F;
                        break;
                    case SV_RING_ICE:
                        if (!one_in_(2))
                            break;
                        o_ptr->name2 = EGO_RING_DRAGON_C;
                        break;
                    case SV_RING_WARNING:
                        if (!one_in_(2))
                            break;
                        o_ptr->name2 = EGO_RING_M_DETECT;
                        break;
                    default:
                        break;
                    }

                    break;
                }
            }

            o_ptr->curse_flags = 0L;
        } else if ((power == -2) && one_in_(2)) {
            if (o_ptr->to_h > 0)
                o_ptr->to_h = 0 - o_ptr->to_h;
            if (o_ptr->to_d > 0)
                o_ptr->to_d = 0 - o_ptr->to_d;
            if (o_ptr->to_a > 0)
                o_ptr->to_a = 0 - o_ptr->to_a;
            if (o_ptr->pval > 0)
                o_ptr->pval = 0 - o_ptr->pval;
            o_ptr->art_flags[0] = 0;
            o_ptr->art_flags[1] = 0;
            while (!o_ptr->name2) {
                object_kind *k_ptr = &k_info[o_ptr->k_idx];
                switch (randint1(5)) {
                case 1:
                    if (has_flag(k_ptr->flags, TR_DRAIN_EXP))
                        break;
                    o_ptr->name2 = EGO_RING_DRAIN_EXP;
                    break;
                case 2:
                    o_ptr->name2 = EGO_RING_NO_MELEE;
                    break;
                case 3:
                    if (has_flag(k_ptr->flags, TR_AGGRAVATE))
                        break;
                    o_ptr->name2 = EGO_RING_AGGRAVATE;
                    break;
                case 4:
                    if (has_flag(k_ptr->flags, TR_TY_CURSE))
                        break;
                    o_ptr->name2 = EGO_RING_TY_CURSE;
                    break;
                case 5:
                    o_ptr->name2 = EGO_RING_ALBINO;
                    break;
                }
            }

            o_ptr->ident |= (IDENT_BROKEN);
            o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
        }

        break;
    }
    case TV_AMULET: {
        switch (o_ptr->sval) {
        case SV_AMULET_INTELLIGENCE:
        case SV_AMULET_WISDOM:
        case SV_AMULET_CHARISMA: {
            o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= (TRC_CURSED);
                o_ptr->pval = 0 - o_ptr->pval;
            }

            break;
        }
        case SV_AMULET_BRILLIANCE: {
            o_ptr->pval = 1 + m_bonus(3, level);
            if (one_in_(4))
                o_ptr->pval++;

            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= (TRC_CURSED);
                o_ptr->pval = 0 - o_ptr->pval;
            }

            break;
        }
        case SV_AMULET_NO_MAGIC:
        case SV_AMULET_NO_TELE: {
            if (power < 0) {
                o_ptr->curse_flags |= (TRC_CURSED);
            }

            break;
        }
        case SV_AMULET_RESISTANCE: {
            if (one_in_(5))
                one_high_resistance(o_ptr);
            if (one_in_(5))
                add_flag(o_ptr->art_flags, TR_RES_POIS);
            break;
        }
        case SV_AMULET_SEARCHING: {
            o_ptr->pval = randint1(2) + (PARAMETER_VALUE)m_bonus(4, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= (TRC_CURSED);
                o_ptr->pval = 0 - (o_ptr->pval);
            }

            break;
        }
        case SV_AMULET_THE_MAGI: {
            o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, level);
            o_ptr->to_a = randint1(5) + (ARMOUR_CLASS)m_bonus(5, level);
            add_esp_weak(o_ptr, FALSE);
            break;
        }
        case SV_AMULET_DOOM: {
            o_ptr->ident |= (IDENT_BROKEN);
            o_ptr->curse_flags |= (TRC_CURSED);
            o_ptr->pval = 0 - (randint1(5) + (PARAMETER_VALUE)m_bonus(5, level));
            o_ptr->to_a = 0 - (randint1(5) + (ARMOUR_CLASS)m_bonus(5, level));
            if (power > 0)
                power = 0 - power;

            break;
        }
        case SV_AMULET_MAGIC_MASTERY: {
            o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(4, level);
            if (power < 0) {
                o_ptr->ident |= (IDENT_BROKEN);
                o_ptr->curse_flags |= (TRC_CURSED);
                o_ptr->pval = 0 - o_ptr->pval;
            }

            break;
        }
        }

        /* power > 2 is debug only */
        if ((one_in_(150) && (power > 0) && !object_is_cursed(o_ptr) && (level > 79)) || (power > 2)) {
            o_ptr->pval = MIN(o_ptr->pval, 4);
            become_random_artifact(owner_ptr, o_ptr, FALSE);
        } else if ((power == 2) && one_in_(2)) {
            while (!o_ptr->name2) {
                object_kind *k_ptr = &k_info[o_ptr->k_idx];
                switch (randint1(21)) {
                case 1:
                case 2:
                    if (has_flag(k_ptr->flags, TR_SLOW_DIGEST))
                        break;
                    o_ptr->name2 = EGO_AMU_SLOW_D;
                    break;
                case 3:
                case 4:
                    if (o_ptr->pval)
                        break;
                    o_ptr->name2 = EGO_AMU_INFRA;
                    break;
                case 5:
                case 6:
                    if (has_flag(k_ptr->flags, TR_SEE_INVIS))
                        break;
                    o_ptr->name2 = EGO_AMU_SEE_INVIS;
                    break;
                case 7:
                case 8:
                    if (has_flag(k_ptr->flags, TR_HOLD_EXP))
                        break;
                    o_ptr->name2 = EGO_AMU_HOLD_EXP;
                    break;
                case 9:
                    if (has_flag(k_ptr->flags, TR_LEVITATION))
                        break;
                    o_ptr->name2 = EGO_AMU_LEVITATION;
                    break;
                case 10:
                case 11:
                case 21:
                    o_ptr->name2 = EGO_AMU_AC;
                    break;
                case 12:
                    if (has_flag(k_ptr->flags, TR_RES_FIRE))
                        break;
                    if (m_bonus(10, level) > 8)
                        o_ptr->name2 = EGO_AMU_RES_FIRE_;
                    else
                        o_ptr->name2 = EGO_AMU_RES_FIRE;
                    break;
                case 13:
                    if (has_flag(k_ptr->flags, TR_RES_COLD))
                        break;
                    if (m_bonus(10, level) > 8)
                        o_ptr->name2 = EGO_AMU_RES_COLD_;
                    else
                        o_ptr->name2 = EGO_AMU_RES_COLD;
                    break;
                case 14:
                    if (has_flag(k_ptr->flags, TR_RES_ELEC))
                        break;
                    if (m_bonus(10, level) > 8)
                        o_ptr->name2 = EGO_AMU_RES_ELEC_;
                    else
                        o_ptr->name2 = EGO_AMU_RES_ELEC;
                    break;
                case 15:
                    if (has_flag(k_ptr->flags, TR_RES_ACID))
                        break;
                    if (m_bonus(10, level) > 8)
                        o_ptr->name2 = EGO_AMU_RES_ACID_;
                    else
                        o_ptr->name2 = EGO_AMU_RES_ACID;
                    break;
                case 16:
                case 17:
                case 18:
                case 19:
                case 20:
                    switch (o_ptr->sval) {
                    case SV_AMULET_TELEPORT:
                        if (m_bonus(10, level) > 9)
                            o_ptr->name2 = EGO_AMU_D_DOOR;
                        else if (one_in_(2))
                            o_ptr->name2 = EGO_AMU_JUMP;
                        else
                            o_ptr->name2 = EGO_AMU_TELEPORT;
                        break;
                    case SV_AMULET_RESIST_ACID:
                        if ((m_bonus(10, level) > 6) && one_in_(2))
                            o_ptr->name2 = EGO_AMU_RES_ACID_;
                        break;
                    case SV_AMULET_SEARCHING:
                        o_ptr->name2 = EGO_AMU_STEALTH;
                        break;
                    case SV_AMULET_BRILLIANCE:
                        if (!one_in_(3))
                            break;
                        o_ptr->name2 = EGO_AMU_IDENT;
                        break;
                    case SV_AMULET_CHARISMA:
                        if (!one_in_(3))
                            break;
                        o_ptr->name2 = EGO_AMU_CHARM;
                        break;
                    case SV_AMULET_THE_MAGI:
                        if (one_in_(2))
                            break;
                        o_ptr->name2 = EGO_AMU_GREAT;
                        break;
                    case SV_AMULET_RESISTANCE:
                        if (!one_in_(5))
                            break;
                        o_ptr->name2 = EGO_AMU_DEFENDER;
                        break;
                    case SV_AMULET_TELEPATHY:
                        if (!one_in_(3))
                            break;
                        o_ptr->name2 = EGO_AMU_DETECTION;
                        break;
                    }
                }
            }
            o_ptr->curse_flags = 0L;
        } else if ((power == -2) && one_in_(2)) {
            if (o_ptr->to_h > 0)
                o_ptr->to_h = 0 - o_ptr->to_h;
            if (o_ptr->to_d > 0)
                o_ptr->to_d = 0 - o_ptr->to_d;
            if (o_ptr->to_a > 0)
                o_ptr->to_a = 0 - o_ptr->to_a;
            if (o_ptr->pval > 0)
                o_ptr->pval = 0 - o_ptr->pval;
            o_ptr->art_flags[0] = 0;
            o_ptr->art_flags[1] = 0;
            while (!o_ptr->name2) {
                object_kind *k_ptr = &k_info[o_ptr->k_idx];
                switch (randint1(5)) {
                case 1:
                    if (has_flag(k_ptr->flags, TR_DRAIN_EXP))
                        break;
                    o_ptr->name2 = EGO_AMU_DRAIN_EXP;
                    break;
                case 2:
                    o_ptr->name2 = EGO_AMU_FOOL;
                    break;
                case 3:
                    if (has_flag(k_ptr->flags, TR_AGGRAVATE))
                        break;
                    o_ptr->name2 = EGO_AMU_AGGRAVATE;
                    break;
                case 4:
                    if (has_flag(k_ptr->flags, TR_TY_CURSE))
                        break;
                    o_ptr->name2 = EGO_AMU_TY_CURSE;
                    break;
                case 5:
                    o_ptr->name2 = EGO_AMU_NAIVETY;
                    break;
                }
            }

            o_ptr->ident |= (IDENT_BROKEN);
            o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
        }

        break;
    }
    }
}
