/*!
 * @brief 防具系のアイテムを強化して(恐らく床に)生成する処理
 * @date 2020/06/02
 * @author Hourier
 * @todo かなり長い。要分割
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
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

AccessoryEnchanter::AccessoryEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
{
    this->owner_ptr = owner_ptr;
    this->o_ptr = o_ptr;
    this->level = level;
    this->power = power;
}

/*!
 * @brief 装飾品系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "ring" or "amulet"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details power > 2 is debug only
 */
void AccessoryEnchanter::apply_magic_accessary()
{
    switch (this->o_ptr->tval) {
    case TV_RING:
        enahcnt_ring();
        if ((one_in_(400) && (this->power > 0) && !object_is_cursed(this->o_ptr) && (this->level > 79)) || (this->power > 2)) {
            this->o_ptr->pval = MIN(this->o_ptr->pval, 4);
            become_random_artifact(this->owner_ptr, this->o_ptr, FALSE);
            break;
        }

        if ((this->power == 2) && one_in_(2)) {
            give_ring_ego_index();
            this->o_ptr->curse_flags = 0L;
            break;
        }

        if ((this->power == -2) && one_in_(2)) {
            give_ring_cursed();
        }

        break;
    case TV_AMULET: {
        enchant_amulet();
        if ((one_in_(150) && (this->power > 0) && !object_is_cursed(this->o_ptr) && (this->level > 79)) || (this->power > 2)) {
            this->o_ptr->pval = MIN(this->o_ptr->pval, 4);
            become_random_artifact(owner_ptr, this->o_ptr, FALSE);
        } else if ((this->power == 2) && one_in_(2)) {
            give_amulet_ego_index();
            this->o_ptr->curse_flags = 0L;
        } else if ((this->power == -2) && one_in_(2)) {
            if (this->o_ptr->to_h > 0)
                this->o_ptr->to_h = 0 - this->o_ptr->to_h;
            if (this->o_ptr->to_d > 0)
                this->o_ptr->to_d = 0 - this->o_ptr->to_d;
            if (this->o_ptr->to_a > 0)
                this->o_ptr->to_a = 0 - this->o_ptr->to_a;
            if (this->o_ptr->pval > 0)
                this->o_ptr->pval = 0 - this->o_ptr->pval;
            this->o_ptr->art_flags[0] = 0;
            this->o_ptr->art_flags[1] = 0;
            while (!this->o_ptr->name2) {
                object_kind *k_ptr = &k_info[this->o_ptr->k_idx];
                switch (randint1(5)) {
                case 1:
                    if (has_flag(k_ptr->flags, TR_DRAIN_EXP))
                        break;
                    this->o_ptr->name2 = EGO_AMU_DRAIN_EXP;
                    break;
                case 2:
                    this->o_ptr->name2 = EGO_AMU_FOOL;
                    break;
                case 3:
                    if (has_flag(k_ptr->flags, TR_AGGRAVATE))
                        break;
                    this->o_ptr->name2 = EGO_AMU_AGGRAVATE;
                    break;
                case 4:
                    if (has_flag(k_ptr->flags, TR_TY_CURSE))
                        break;
                    this->o_ptr->name2 = EGO_AMU_TY_CURSE;
                    break;
                case 5:
                    this->o_ptr->name2 = EGO_AMU_NAIVETY;
                    break;
                }
            }

            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
        }

        break;
    }

    default:
        break;
    }
}

void AccessoryEnchanter::enahcnt_ring()
{
    switch (this->o_ptr->sval) {
    case SV_RING_ATTACKS:
        this->o_ptr->pval = (PARAMETER_VALUE)m_bonus(2, this->level);
        if (one_in_(15)) {
            this->o_ptr->pval++;
        }

        if (this->o_ptr->pval < 1) {
            this->o_ptr->pval = 1;
        }

        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->pval = 0 - (this->o_ptr->pval);
        }

        break;
    case SV_RING_SHOTS:
        break;
    case SV_RING_STR:
    case SV_RING_CON:
    case SV_RING_DEX:
        this->o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, this->level);
        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->pval = 0 - (this->o_ptr->pval);
        }

        break;
    case SV_RING_SPEED:
        this->o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, this->level);
        while (randint0(100) < 50) {
            this->o_ptr->pval++;
        }

        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->pval = 0 - (this->o_ptr->pval);
            break;
        }

        break;
    case SV_RING_LORDLY:
        do {
            one_lordly_high_resistance(this->o_ptr);
        } while (one_in_(4));

        this->o_ptr->to_a = 10 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, this->level);
        break;
    case SV_RING_WARNING:
        if (one_in_(3)) {
            one_low_esp(this->o_ptr);
        }

        break;
    case SV_RING_SEARCHING:
        this->o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, this->level);
        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->pval = 0 - (this->o_ptr->pval);
        }

        break;
    case SV_RING_FLAMES:
    case SV_RING_ACID:
    case SV_RING_ICE:
    case SV_RING_ELEC:
        this->o_ptr->to_a = 5 + randint1(5) + (ARMOUR_CLASS)m_bonus(10, this->level);
        break;
    case SV_RING_WEAKNESS:
    case SV_RING_STUPIDITY:
        this->o_ptr->ident |= (IDENT_BROKEN);
        this->o_ptr->curse_flags |= TRC_CURSED;
        this->o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, this->level));
        if (this->power > 0) {
            this->power = 0 - this->power;
        }

        break;
    case SV_RING_WOE:
        this->o_ptr->ident |= (IDENT_BROKEN);
        this->o_ptr->curse_flags |= TRC_CURSED;
        this->o_ptr->to_a = 0 - (5 + (ARMOUR_CLASS)m_bonus(10, this->level));
        this->o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, this->level));
        if (this->power > 0) {
            this->power = 0 - this->power;
        }

        break;
    case SV_RING_DAMAGE:
        this->o_ptr->to_d = 1 + randint1(5) + (HIT_POINT)m_bonus(16, this->level);
        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->to_d = 0 - this->o_ptr->to_d;
        }

        break;
    case SV_RING_ACCURACY:
        this->o_ptr->to_h = 1 + randint1(5) + (HIT_PROB)m_bonus(16, this->level);
        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->to_h = 0 - this->o_ptr->to_h;
        }

        break;
    case SV_RING_PROTECTION:
        this->o_ptr->to_a = 5 + randint1(8) + (ARMOUR_CLASS)m_bonus(10, this->level);
        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->to_a = 0 - this->o_ptr->to_a;
        }

        break;
    case SV_RING_SLAYING:
        this->o_ptr->to_d = randint1(5) + (HIT_POINT)m_bonus(12, this->level);
        this->o_ptr->to_h = randint1(5) + (HIT_PROB)m_bonus(12, this->level);

        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->to_h = 0 - this->o_ptr->to_h;
            this->o_ptr->to_d = 0 - this->o_ptr->to_d;
        }

        break;
    case SV_RING_MUSCLE:
        this->o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(3, this->level);
        if (one_in_(4)) {
            this->o_ptr->pval++;
        }

        if (this->power < 0) {
            this->o_ptr->ident |= (IDENT_BROKEN);
            this->o_ptr->curse_flags |= TRC_CURSED;
            this->o_ptr->pval = 0 - this->o_ptr->pval;
        }

        break;
    case SV_RING_AGGRAVATION:
        this->o_ptr->ident |= (IDENT_BROKEN);
        this->o_ptr->curse_flags |= TRC_CURSED;
        if (this->power > 0) {
            this->power = 0 - this->power;
        }

        break;
    default:
        break;
    }
}

void AccessoryEnchanter::give_ring_ego_index()
{
    while (!this->o_ptr->name2) {
        int tmp = m_bonus(10, this->level);
        object_kind *k_ptr = &k_info[this->o_ptr->k_idx];
        switch (randint1(28)) {
        case 1:
        case 2:
            this->o_ptr->name2 = EGO_RING_THROW;
            break;
        case 3:
        case 4:
            if (has_flag(k_ptr->flags, TR_REGEN)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_REGEN;
            break;
        case 5:
        case 6:
            if (has_flag(k_ptr->flags, TR_LITE_1)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_LITE;
            break;
        case 7:
        case 8:
            if (has_flag(k_ptr->flags, TR_TELEPORT)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_TELEPORT;
            break;
        case 9:
        case 10:
            if (this->o_ptr->to_h) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_TO_H;
            break;
        case 11:
        case 12:
            if (this->o_ptr->to_d) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_TO_D;
            break;
        case 13:
            if ((this->o_ptr->to_h) || (this->o_ptr->to_d)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_SLAY;
            break;
        case 14:
            if ((has_flag(k_ptr->flags, TR_STR)) || this->o_ptr->to_h || this->o_ptr->to_d) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_WIZARD;
            break;
        case 15:
            if (has_flag(k_ptr->flags, TR_ACTIVATE)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_HERO;
            break;
        case 16:
            if (has_flag(k_ptr->flags, TR_ACTIVATE)) {
                break;
            }

            if (tmp > 8) {
                this->o_ptr->name2 = EGO_RING_MANA_BALL;
                break;
            }
            
            if (tmp > 4) {
                this->o_ptr->name2 = EGO_RING_MANA_BOLT;
                break;
            }
            
            this->o_ptr->name2 = EGO_RING_MAGIC_MIS;            
            break;
        case 17:
            if (has_flag(k_ptr->flags, TR_ACTIVATE)) {
                break;
            }

            if (!(has_flag(k_ptr->flags, TR_RES_FIRE))
                && (has_flag(k_ptr->flags, TR_RES_COLD) || has_flag(k_ptr->flags, TR_RES_ELEC) || has_flag(k_ptr->flags, TR_RES_ACID))) {
                break;
            }

            if (tmp > 7) {
                this->o_ptr->name2 = EGO_RING_DRAGON_F;
                break;
            }

            if (tmp > 3) {
                this->o_ptr->name2 = EGO_RING_FIRE_BALL;
                break;
            }

            this->o_ptr->name2 = EGO_RING_FIRE_BOLT;
            break;
        case 18:
            if (has_flag(k_ptr->flags, TR_ACTIVATE)) {
                break;
            }

            if (!(has_flag(k_ptr->flags, TR_RES_COLD))
                && (has_flag(k_ptr->flags, TR_RES_FIRE) || has_flag(k_ptr->flags, TR_RES_ELEC) || has_flag(k_ptr->flags, TR_RES_ACID))) {
                break;
            }

            if (tmp > 7) {
                this->o_ptr->name2 = EGO_RING_DRAGON_C;
                break;
            }

            if (tmp > 3) {
                this->o_ptr->name2 = EGO_RING_COLD_BALL;
                break;
            }

            this->o_ptr->name2 = EGO_RING_COLD_BOLT;
            break;
        case 19:
            if (has_flag(k_ptr->flags, TR_ACTIVATE)) {
                break;
            }

            if (!(has_flag(k_ptr->flags, TR_RES_ELEC))
                && (has_flag(k_ptr->flags, TR_RES_COLD) || has_flag(k_ptr->flags, TR_RES_FIRE) || has_flag(k_ptr->flags, TR_RES_ACID))) {
                break;
            }

            if (tmp > 4) {
                this->o_ptr->name2 = EGO_RING_ELEC_BALL;
                break;
            }

            this->o_ptr->name2 = EGO_RING_ELEC_BOLT;
            break;
        case 20:
            if (has_flag(k_ptr->flags, TR_ACTIVATE)) {
                break;
            }

            if (!(has_flag(k_ptr->flags, TR_RES_ACID))
                && (has_flag(k_ptr->flags, TR_RES_COLD) || has_flag(k_ptr->flags, TR_RES_ELEC) || has_flag(k_ptr->flags, TR_RES_FIRE))) {
                break;
            }

            if (tmp > 4) {
                this->o_ptr->name2 = EGO_RING_ACID_BALL;
                break;
            }

            this->o_ptr->name2 = EGO_RING_ACID_BOLT;
            break;
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
            give_ring_high_ego_index();
            break;
        }
    }
}

void AccessoryEnchanter::give_ring_high_ego_index()
{
    switch (this->o_ptr->sval) {
    case SV_RING_SPEED:
        if (!one_in_(3)) {
            break;
        }

        this->o_ptr->name2 = EGO_RING_D_SPEED;
        break;
    case SV_RING_DAMAGE:
    case SV_RING_ACCURACY:
    case SV_RING_SLAYING:
        if (next_bool()) {
            break;
        }

        if (one_in_(2)) {
            this->o_ptr->name2 = EGO_RING_HERO;
            break;
        }

        this->o_ptr->name2 = EGO_RING_BERSERKER;
        this->o_ptr->to_h -= 2 + randint1(4);
        this->o_ptr->to_d += 2 + randint1(4);
        break;
    case SV_RING_PROTECTION:
        this->o_ptr->name2 = EGO_RING_SUPER_AC;
        this->o_ptr->to_a += 7 + m_bonus(5, this->level);
        break;
    case SV_RING_RES_FEAR:
        this->o_ptr->name2 = EGO_RING_HERO;
        break;
    case SV_RING_SHOTS:
        if (next_bool()) {
            break;
        }

        this->o_ptr->name2 = EGO_RING_HUNTER;
        break;
    case SV_RING_SEARCHING:
        this->o_ptr->name2 = EGO_RING_STEALTH;
        break;
    case SV_RING_TELEPORTATION:
        this->o_ptr->name2 = EGO_RING_TELE_AWAY;
        break;
    case SV_RING_RES_BLINDNESS:
        this->o_ptr->name2 = next_bool() ? EGO_RING_RES_LITE : EGO_RING_RES_DARK;
        break;
    case SV_RING_LORDLY:
        if (!one_in_(20)) {
            break;
        }

        one_lordly_high_resistance(this->o_ptr);
        one_lordly_high_resistance(this->o_ptr);
        this->o_ptr->name2 = EGO_RING_TRUE;
        break;
    case SV_RING_FLAMES:
        if (next_bool()) {
            break;
        }

        this->o_ptr->name2 = EGO_RING_DRAGON_F;
        break;
    case SV_RING_ICE:
        if (next_bool()) {
            break;
        }

        this->o_ptr->name2 = EGO_RING_DRAGON_C;
        break;
    case SV_RING_WARNING:
        if (next_bool()) {
            break;
        }

        this->o_ptr->name2 = EGO_RING_M_DETECT;
        break;
    default:
        break;
    }
}

void AccessoryEnchanter::give_ring_cursed()
{
    if (this->o_ptr->to_h > 0) {
        this->o_ptr->to_h = 0 - this->o_ptr->to_h;
    }

    if (this->o_ptr->to_d > 0) {
        this->o_ptr->to_d = 0 - this->o_ptr->to_d;
    }

    if (this->o_ptr->to_a > 0) {
        this->o_ptr->to_a = 0 - this->o_ptr->to_a;
    }

    if (this->o_ptr->pval > 0) {
        this->o_ptr->pval = 0 - this->o_ptr->pval;
    }

    this->o_ptr->art_flags[0] = 0;
    this->o_ptr->art_flags[1] = 0;
    while (!this->o_ptr->name2) {
        object_kind *k_ptr = &k_info[this->o_ptr->k_idx];
        switch (randint1(5)) {
        case 1:
            if (has_flag(k_ptr->flags, TR_DRAIN_EXP))
                break;
            this->o_ptr->name2 = EGO_RING_DRAIN_EXP;
            break;
        case 2:
            this->o_ptr->name2 = EGO_RING_NO_MELEE;
            break;
        case 3:
            if (has_flag(k_ptr->flags, TR_AGGRAVATE))
                break;
            this->o_ptr->name2 = EGO_RING_AGGRAVATE;
            break;
        case 4:
            if (has_flag(k_ptr->flags, TR_TY_CURSE))
                break;
            this->o_ptr->name2 = EGO_RING_TY_CURSE;
            break;
        case 5:
            this->o_ptr->name2 = EGO_RING_ALBINO;
            break;
        }
    }

    set_bits(this->o_ptr->ident, IDENT_BROKEN);
    set_bits(this->o_ptr->curse_flags, TRC_CURSED | TRC_HEAVY_CURSE);
}

void AccessoryEnchanter::enchant_amulet()
{
    switch (this->o_ptr->sval) {
    case SV_AMULET_INTELLIGENCE:
    case SV_AMULET_WISDOM:
    case SV_AMULET_CHARISMA:
        this->o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, this->level);
        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            set_bits(this->o_ptr->curse_flags, TRC_CURSED);
            this->o_ptr->pval = 0 - this->o_ptr->pval;
        }

        break;
    case SV_AMULET_BRILLIANCE:
        this->o_ptr->pval = 1 + m_bonus(3, this->level);
        if (one_in_(4)) {
            this->o_ptr->pval++;
        }

        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            set_bits(this->o_ptr->curse_flags, TRC_CURSED);
            this->o_ptr->pval = 0 - this->o_ptr->pval;
        }

        break;
    case SV_AMULET_NO_MAGIC:
    case SV_AMULET_NO_TELE:
        if (this->power < 0) {
            set_bits(this->o_ptr->curse_flags, TRC_CURSED);
        }

        break;
    case SV_AMULET_RESISTANCE:
        if (one_in_(5)) {
            one_high_resistance(this->o_ptr);
        }

        if (one_in_(5)) {
            add_flag(this->o_ptr->art_flags, TR_RES_POIS);
        }

        break;
    case SV_AMULET_SEARCHING:
        this->o_ptr->pval = 2 + randint1(6);
        if (this->power >= 0) {
            add_esp_weak(this->o_ptr, FALSE);
            break;
        }

        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        set_bits(this->o_ptr->curse_flags, TRC_CURSED);
        this->o_ptr->pval = 0 - (this->o_ptr->pval);
        break;
    case SV_AMULET_THE_MAGI:
        this->o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, this->level);
        this->o_ptr->to_a = randint1(5) + (ARMOUR_CLASS)m_bonus(5, this->level);
        add_esp_weak(this->o_ptr, FALSE);
        break;
    case SV_AMULET_DOOM:
        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        set_bits(this->o_ptr->curse_flags, TRC_CURSED);
        this->o_ptr->pval = 0 - (randint1(5) + (PARAMETER_VALUE)m_bonus(5, this->level));
        this->o_ptr->to_a = 0 - (randint1(5) + (ARMOUR_CLASS)m_bonus(5, this->level));
        if (this->power > 0) {
            this->power = 0 - this->power;
        }

        break;
    case SV_AMULET_MAGIC_MASTERY:
        this->o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(4, this->level);
        if (this->power >= 0) {
            break;
        }

        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        set_bits(this->o_ptr->curse_flags, TRC_CURSED);
        this->o_ptr->pval = 0 - this->o_ptr->pval;
        break;
    }
}

void AccessoryEnchanter::give_amulet_ego_index()
{
    while (!this->o_ptr->name2) {
        object_kind *k_ptr = &k_info[this->o_ptr->k_idx];
        switch (randint1(21)) {
        case 1:
        case 2:
            if (has_flag(k_ptr->flags, TR_SLOW_DIGEST)) {
                break;
            }

            this->o_ptr->name2 = EGO_AMU_SLOW_D;
            break;
        case 3:
        case 4:
            if (this->o_ptr->pval) {
                break;
            }
            
            this->o_ptr->name2 = EGO_AMU_INFRA;
            break;
        case 5:
        case 6:
            if (has_flag(k_ptr->flags, TR_SEE_INVIS)) {
                break;
            }

            this->o_ptr->name2 = EGO_AMU_SEE_INVIS;
            break;
        case 7:
        case 8:
            if (has_flag(k_ptr->flags, TR_HOLD_EXP)) {
                break;
            }

            this->o_ptr->name2 = EGO_AMU_HOLD_EXP;
            break;
        case 9:
            if (has_flag(k_ptr->flags, TR_LEVITATION)) {
                break;
            }

            this->o_ptr->name2 = EGO_AMU_LEVITATION;
            break;
        case 10:
        case 11:
        case 21:
            this->o_ptr->name2 = EGO_AMU_AC;
            break;
        case 12:
            if (has_flag(k_ptr->flags, TR_RES_FIRE)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->name2 = EGO_AMU_RES_FIRE_;
                break;
            }
            
            this->o_ptr->name2 = EGO_AMU_RES_FIRE;
            break;
        case 13:
            if (has_flag(k_ptr->flags, TR_RES_COLD)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->name2 = EGO_AMU_RES_COLD_;
                break;
            }

            
            this->o_ptr->name2 = EGO_AMU_RES_COLD;
            break;
        case 14:
            if (has_flag(k_ptr->flags, TR_RES_ELEC)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->name2 = EGO_AMU_RES_ELEC_;
                break;
            }
            
            this->o_ptr->name2 = EGO_AMU_RES_ELEC;
            break;
        case 15:
            if (has_flag(k_ptr->flags, TR_RES_ACID)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->name2 = EGO_AMU_RES_ACID_;
                break;
            }
            
            this->o_ptr->name2 = EGO_AMU_RES_ACID;
            break;
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
            give_amulet_high_ego_index();
            break;
        }
    }
}

void AccessoryEnchanter::give_amulet_high_ego_index()
{
    switch (this->o_ptr->sval) {
    case SV_AMULET_TELEPORT:
        if (m_bonus(10, this->level) > 9) {
            this->o_ptr->name2 = EGO_AMU_D_DOOR;
            break;
        }
        
        if (one_in_(2)) {
            this->o_ptr->name2 = EGO_AMU_JUMP;
            break;
        }
        
        this->o_ptr->name2 = EGO_AMU_TELEPORT;
        break;
    case SV_AMULET_RESIST_ACID:
        if ((m_bonus(10, this->level) > 6) && one_in_(2)) {
            this->o_ptr->name2 = EGO_AMU_RES_ACID_;
        }

        break;
    case SV_AMULET_SEARCHING:
        this->o_ptr->name2 = EGO_AMU_STEALTH;
        break;
    case SV_AMULET_BRILLIANCE:
        if (!one_in_(3)) {
            break;
        }

        this->o_ptr->name2 = EGO_AMU_IDENT;
        break;
    case SV_AMULET_CHARISMA:
        if (!one_in_(3)) {
            break;
        }

        this->o_ptr->name2 = EGO_AMU_CHARM;
        break;
    case SV_AMULET_THE_MAGI:
        if (one_in_(2)) {
            break;
        }

        this->o_ptr->name2 = EGO_AMU_GREAT;
        break;
    case SV_AMULET_RESISTANCE:
        if (!one_in_(5)) {
            break;
        }

        this->o_ptr->name2 = EGO_AMU_DEFENDER;
        break;
    case SV_AMULET_TELEPATHY:
        if (!one_in_(3)) {
            break;
        }

        this->o_ptr->name2 = EGO_AMU_DETECTION;
        break;
    default:
        break;
    }
}
