/*!
 * @brief 指輪を強化生成する処理
 * @date 2021/04/30
 * @author Hourier
 */

#include "object-enchant/apply-magic-ring.h"
#include "artifact/random-art-generator.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "object/object-kind.h"
#include "sv-definition/sv-ring-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*
 * @brief コンストラクタ
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
RingEnchanter::RingEnchanter(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
    : owner_ptr(owner_ptr)
    , o_ptr(o_ptr)
    , level(level)
    , power(power)
{
}

/*!
 * @brief 指輪に生成ランクごとの強化を与える
 * Apply magic to an item known to be a "ring" or "amulet"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details power > 2はデバッグ専用.
 */
void RingEnchanter::apply_magic()
{
    if ((this->power == 0) && (randint0(100) < 50)) {
        this->power = -1;
    }

    this->enchant();
    if ((one_in_(400) && (this->power > 0) && !this->o_ptr->is_cursed() && (this->level > 79)) || (this->power > 2)) {
        this->o_ptr->pval = MIN(this->o_ptr->pval, 4);
        become_random_artifact(this->owner_ptr, this->o_ptr, false);
        return;
    }

    if ((this->power == 2) && one_in_(2)) {
        give_ego_index();
        this->o_ptr->curse_flags.clear();
        return;
    }

    if ((this->power == -2) && one_in_(2)) {
        give_cursed();
    }
}

void RingEnchanter::enchant()
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
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
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
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
            this->o_ptr->pval = 0 - (this->o_ptr->pval);
        }

        break;
    case SV_RING_SPEED:
        this->o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, this->level);
        while (randint0(100) < 50) {
            this->o_ptr->pval++;
        }

        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
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
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
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
        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        this->o_ptr->curse_flags.set(TRC::CURSED);
        this->o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, this->level));
        if (this->power > 0) {
            this->power = 0 - this->power;
        }

        break;
    case SV_RING_WOE:
        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        this->o_ptr->curse_flags.set(TRC::CURSED);
        this->o_ptr->to_a = 0 - (5 + (ARMOUR_CLASS)m_bonus(10, this->level));
        this->o_ptr->pval = 0 - (1 + (PARAMETER_VALUE)m_bonus(5, this->level));
        if (this->power > 0) {
            this->power = 0 - this->power;
        }

        break;
    case SV_RING_DAMAGE:
        this->o_ptr->to_d = 1 + randint1(5) + (HIT_POINT)m_bonus(16, this->level);
        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
            this->o_ptr->to_d = 0 - this->o_ptr->to_d;
        }

        break;
    case SV_RING_ACCURACY:
        this->o_ptr->to_h = 1 + randint1(5) + (HIT_PROB)m_bonus(16, this->level);
        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
            this->o_ptr->to_h = 0 - this->o_ptr->to_h;
        }

        break;
    case SV_RING_PROTECTION:
        this->o_ptr->to_a = 5 + randint1(8) + (ARMOUR_CLASS)m_bonus(10, this->level);
        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
            this->o_ptr->to_a = 0 - this->o_ptr->to_a;
        }

        break;
    case SV_RING_SLAYING:
        this->o_ptr->to_d = randint1(5) + (HIT_POINT)m_bonus(12, this->level);
        this->o_ptr->to_h = randint1(5) + (HIT_PROB)m_bonus(12, this->level);

        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
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
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(TRC::CURSED);
            this->o_ptr->pval = 0 - this->o_ptr->pval;
        }

        break;
    case SV_RING_AGGRAVATION:
        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        this->o_ptr->curse_flags.set(TRC::CURSED);
        if (this->power > 0) {
            this->power = 0 - this->power;
        }

        break;
    default:
        break;
    }
}

void RingEnchanter::give_ego_index()
{
    while (!this->o_ptr->name2) {
        int tmp = m_bonus(10, this->level);
        auto *k_ptr = &k_info[this->o_ptr->k_idx];
        switch (randint1(28)) {
        case 1:
        case 2:
            this->o_ptr->name2 = EGO_RING_THROW;
            break;
        case 3:
        case 4:
            if (k_ptr->flags.has(TR_REGEN)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_REGEN;
            break;
        case 5:
        case 6:
            if (k_ptr->flags.has(TR_LITE_1)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_LITE;
            break;
        case 7:
        case 8:
            if (k_ptr->flags.has(TR_TELEPORT)) {
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
            if ((k_ptr->flags.has(TR_STR)) || this->o_ptr->to_h || this->o_ptr->to_d) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_WIZARD;
            break;
        case 15:
            if (k_ptr->flags.has(TR_ACTIVATE)) {
                break;
            }

            this->o_ptr->name2 = EGO_RING_HERO;
            break;
        case 16:
            if (k_ptr->flags.has(TR_ACTIVATE)) {
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
            if (k_ptr->flags.has(TR_ACTIVATE)) {
                break;
            }

            if (k_ptr->flags.has_not(TR_RES_FIRE) && (k_ptr->flags.has(TR_RES_COLD) || k_ptr->flags.has(TR_RES_ELEC) || k_ptr->flags.has(TR_RES_ACID))) {
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
            if (k_ptr->flags.has(TR_ACTIVATE)) {
                break;
            }

            if (k_ptr->flags.has_not(TR_RES_COLD) && (k_ptr->flags.has(TR_RES_FIRE) || k_ptr->flags.has(TR_RES_ELEC) || k_ptr->flags.has(TR_RES_ACID))) {
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
            if (k_ptr->flags.has(TR_ACTIVATE)) {
                break;
            }

            if (k_ptr->flags.has_not(TR_RES_ELEC) && (k_ptr->flags.has(TR_RES_COLD) || k_ptr->flags.has(TR_RES_FIRE) || k_ptr->flags.has(TR_RES_ACID))) {
                break;
            }

            if (tmp > 4) {
                this->o_ptr->name2 = EGO_RING_ELEC_BALL;
                break;
            }

            this->o_ptr->name2 = EGO_RING_ELEC_BOLT;
            break;
        case 20:
            if (k_ptr->flags.has(TR_ACTIVATE)) {
                break;
            }

            if (k_ptr->flags.has_not(TR_RES_ACID) && (k_ptr->flags.has(TR_RES_COLD) || k_ptr->flags.has(TR_RES_ELEC) || k_ptr->flags.has(TR_RES_FIRE))) {
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
            give_high_ego_index();
            break;
        }
    }
}

void RingEnchanter::give_high_ego_index()
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

void RingEnchanter::give_cursed()
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

    while (!this->o_ptr->name2) {
        auto *k_ptr = &k_info[this->o_ptr->k_idx];
        switch (randint1(5)) {
        case 1:
            if (k_ptr->flags.has(TR_DRAIN_EXP))
                break;
            this->o_ptr->name2 = EGO_RING_DRAIN_EXP;
            break;
        case 2:
            this->o_ptr->name2 = EGO_RING_NO_MELEE;
            break;
        case 3:
            if (k_ptr->flags.has(TR_AGGRAVATE))
                break;
            this->o_ptr->name2 = EGO_RING_AGGRAVATE;
            break;
        case 4:
            if (k_ptr->flags.has(TR_TY_CURSE))
                break;
            this->o_ptr->name2 = EGO_RING_TY_CURSE;
            break;
        case 5:
            this->o_ptr->name2 = EGO_RING_ALBINO;
            break;
        }
    }

    set_bits(this->o_ptr->ident, IDENT_BROKEN);
    this->o_ptr->curse_flags.set({ TRC::CURSED, TRC::HEAVY_CURSE });
}
