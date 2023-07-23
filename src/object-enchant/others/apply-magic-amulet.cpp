/*!
 * @brief アミュレットを強化生成する処理
 * @date 2021/04/30
 * @author Hourier
 */

#include "object-enchant/others/apply-magic-amulet.h"
#include "artifact/random-art-generator.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trc-types.h"
#include "sv-definition/sv-amulet-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
AmuletEnchanter::AmuletEnchanter(PlayerType *player_ptr, ItemEntity *o_ptr, DEPTH level, int power)
    : player_ptr(player_ptr)
    , o_ptr(o_ptr)
    , level(level)
    , power(power)
{
}

/*!
 * @brief アミュレットに生成ランクごとの強化を与える
 * Apply magic to an item known to be a "ring" or "amulet"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details power > 2はデバッグ専用.
 */
void AmuletEnchanter::apply_magic()
{
    if ((this->power == 0) && (randint0(100) < 50)) {
        this->power = -1;
    }

    this->sval_enchant();
    if ((this->power > 2) || (one_in_(150) && (this->power > 0) && !this->o_ptr->is_cursed() && (this->level > 79))) {
        this->o_ptr->pval = std::min<short>(this->o_ptr->pval, 4);
        become_random_artifact(player_ptr, this->o_ptr, false);
        return;
    }

    if ((this->power == 2) && one_in_(2)) {
        give_ego_index();
        this->o_ptr->curse_flags.clear();
        return;
    }

    if ((this->power == -2) && one_in_(2)) {
        give_cursed();
        return;
    }
}

void AmuletEnchanter::sval_enchant()
{
    const auto sval = this->o_ptr->bi_key.sval();
    if (!sval.has_value()) {
        return;
    }

    switch (sval.value()) {
    case SV_AMULET_INTELLIGENCE:
    case SV_AMULET_WISDOM:
    case SV_AMULET_CHARISMA:
        this->o_ptr->pval = 1 + (PARAMETER_VALUE)m_bonus(5, this->level);
        if (this->power < 0) {
            set_bits(this->o_ptr->ident, IDENT_BROKEN);
            this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
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
            this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
            this->o_ptr->pval = 0 - this->o_ptr->pval;
        }

        break;
    case SV_AMULET_NO_MAGIC:
    case SV_AMULET_NO_TELE:
        if (this->power < 0) {
            this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
        }

        break;
    case SV_AMULET_RESISTANCE:
        if (one_in_(5)) {
            one_high_resistance(this->o_ptr);
        }

        if (one_in_(5)) {
            this->o_ptr->art_flags.set(TR_RES_POIS);
        }

        break;
    case SV_AMULET_SEARCHING:
        this->o_ptr->pval = 2 + randint1(6);
        if (this->power >= 0) {
            add_esp_weak(this->o_ptr, false);
            break;
        }

        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
        this->o_ptr->pval = 0 - (this->o_ptr->pval);
        break;
    case SV_AMULET_THE_MAGI:
        this->o_ptr->pval = randint1(5) + (PARAMETER_VALUE)m_bonus(5, this->level);
        this->o_ptr->to_a = randint1(5) + (ARMOUR_CLASS)m_bonus(5, this->level);
        add_esp_weak(this->o_ptr, false);
        break;
    case SV_AMULET_DOOM:
        set_bits(this->o_ptr->ident, IDENT_BROKEN);
        this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
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
        this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
        this->o_ptr->pval = 0 - this->o_ptr->pval;
        break;
    }
}

void AmuletEnchanter::give_ego_index()
{
    while (!this->o_ptr->is_ego()) {
        const auto &baseitem = this->o_ptr->get_baseitem();
        switch (randint1(21)) {
        case 1:
        case 2:
            if (baseitem.flags.has(TR_SLOW_DIGEST)) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_SLOW_D;
            break;
        case 3:
        case 4:
            if (this->o_ptr->pval) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_INFRA;
            break;
        case 5:
        case 6:
            if (baseitem.flags.has(TR_SEE_INVIS)) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_SEE_INVIS;
            break;
        case 7:
        case 8:
            if (baseitem.flags.has(TR_HOLD_EXP)) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_HOLD_EXP;
            break;
        case 9:
            if (baseitem.flags.has(TR_LEVITATION)) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_LEVITATION;
            break;
        case 10:
        case 11:
        case 21:
            this->o_ptr->ego_idx = EgoType::AMU_AC;
            break;
        case 12:
            if (baseitem.flags.has(TR_RES_FIRE)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->ego_idx = EgoType::AMU_RES_FIRE_;
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_RES_FIRE;
            break;
        case 13:
            if (baseitem.flags.has(TR_RES_COLD)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->ego_idx = EgoType::AMU_RES_COLD_;
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_RES_COLD;
            break;
        case 14:
            if (baseitem.flags.has(TR_RES_ELEC)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->ego_idx = EgoType::AMU_RES_ELEC_;
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_RES_ELEC;
            break;
        case 15:
            if (baseitem.flags.has(TR_RES_ACID)) {
                break;
            }

            if (m_bonus(10, this->level) > 8) {
                this->o_ptr->ego_idx = EgoType::AMU_RES_ACID_;
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_RES_ACID;
            break;
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
            give_high_ego_index();
            break;
        }
    }
}

void AmuletEnchanter::give_high_ego_index()
{
    const auto sval = this->o_ptr->bi_key.sval();
    if (!sval.has_value()) {
        return;
    }

    switch (sval.value()) {
    case SV_AMULET_TELEPORT:
        if (m_bonus(10, this->level) > 9) {
            this->o_ptr->ego_idx = EgoType::AMU_D_DOOR;
            break;
        }

        if (one_in_(2)) {
            this->o_ptr->ego_idx = EgoType::AMU_JUMP;
            break;
        }

        this->o_ptr->ego_idx = EgoType::AMU_TELEPORT;
        break;
    case SV_AMULET_RESIST_ACID:
        if ((m_bonus(10, this->level) > 6) && one_in_(2)) {
            this->o_ptr->ego_idx = EgoType::AMU_RES_ACID_;
        }

        break;
    case SV_AMULET_SEARCHING:
        this->o_ptr->ego_idx = EgoType::AMU_STEALTH;
        break;
    case SV_AMULET_BRILLIANCE:
        if (!one_in_(3)) {
            break;
        }

        this->o_ptr->ego_idx = EgoType::AMU_IDENT;
        break;
    case SV_AMULET_CHARISMA:
        if (!one_in_(3)) {
            break;
        }

        this->o_ptr->ego_idx = EgoType::AMU_CHARM;
        break;
    case SV_AMULET_THE_MAGI:
        if (one_in_(2)) {
            break;
        }

        this->o_ptr->ego_idx = EgoType::AMU_GREAT;
        break;
    case SV_AMULET_RESISTANCE:
        if (!one_in_(5)) {
            break;
        }

        this->o_ptr->ego_idx = EgoType::AMU_DEFENDER;
        break;
    case SV_AMULET_TELEPATHY:
        if (!one_in_(3)) {
            break;
        }

        this->o_ptr->ego_idx = EgoType::AMU_DETECTION;
        break;
    default:
        break;
    }
}

void AmuletEnchanter::give_cursed()
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

    while (!this->o_ptr->is_ego()) {
        const auto &baseitem = this->o_ptr->get_baseitem();
        switch (randint1(5)) {
        case 1:
            if (baseitem.flags.has(TR_DRAIN_EXP)) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_DRAIN_EXP;
            break;
        case 2:
            this->o_ptr->ego_idx = EgoType::AMU_FOOL;
            break;
        case 3:
            if (baseitem.flags.has(TR_AGGRAVATE)) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_AGGRAVATE;
            break;
        case 4:
            if (baseitem.flags.has(TR_TY_CURSE)) {
                break;
            }

            this->o_ptr->ego_idx = EgoType::AMU_TY_CURSE;
            break;
        case 5:
            this->o_ptr->ego_idx = EgoType::AMU_NAIVETY;
            break;
        }
    }

    set_bits(this->o_ptr->ident, IDENT_BROKEN);
    this->o_ptr->curse_flags.set({ CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE });
}
