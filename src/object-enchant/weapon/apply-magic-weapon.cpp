/*!
 * @brief 武器系のアイテムを強化する処理
 * @date 2022/01/30
 * @author Hourier
 */

#include "object-enchant/weapon/apply-magic-weapon.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 武器強化クラスのコンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 */
WeaponEnchanter::WeaponEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power)
    : AbstractWeaponEnchanter(o_ptr, level, power)
    , player_ptr(player_ptr)
{
}

/*!
 * @brief 武器系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "weapon"
 * @details power > 2はデバッグ専用.
 */
void WeaponEnchanter::apply_magic()
{
    if (this->should_skip) {
        return;
    }

    switch (this->o_ptr->tval) {
    case ItemKindType::DIGGING: {
        if (this->power > 1) {
            if ((this->power > 2) || one_in_(30))
                become_random_artifact(this->player_ptr, this->o_ptr, false);
            else
                this->o_ptr->name2 = EgoType::DIGGING;
        } else if (this->power < -1) {
            this->o_ptr->pval = 0 - (5 + randint1(5));
        } else if (this->power < 0) {
            this->o_ptr->pval = 0 - (this->o_ptr->pval);
        }

        break;
    }
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD: {
        if (this->power > 1) {
            if ((this->power > 2) || one_in_(40)) {
                become_random_artifact(this->player_ptr, this->o_ptr, false);
                break;
            }

            while (true) {
                this->o_ptr->name2 = get_random_ego(INVEN_MAIN_HAND, true);
                if (this->o_ptr->name2 == EgoType::SHARPNESS && this->o_ptr->tval != ItemKindType::SWORD) {
                    continue;
                }

                if (this->o_ptr->name2 == EgoType::EARTHQUAKES && this->o_ptr->tval != ItemKindType::HAFTED) {
                    continue;
                }

                break;
            }

            switch (this->o_ptr->name2) {
            case EgoType::SHARPNESS:
                this->o_ptr->pval = (PARAMETER_VALUE)m_bonus(5, this->level) + 1;
                break;
            case EgoType::EARTHQUAKES:
                if (one_in_(3) && (this->level > 60)) {
                    this->o_ptr->art_flags.set(TR_BLOWS);
                } else {
                    this->o_ptr->pval = (PARAMETER_VALUE)m_bonus(3, this->level);
                }

                break;
            default:
                break;
            }

            if (!this->o_ptr->art_name) {
                while (one_in_(10L * this->o_ptr->dd * this->o_ptr->ds)) {
                    this->o_ptr->dd++;
                }

                if (this->o_ptr->dd > 9) {
                    this->o_ptr->dd = 9;
                }
            }

            break;
        }

        if (this->power < -1) {
            if (randint0(MAX_DEPTH) < this->level) {
                auto n = 0;
                while (true) {
                    this->o_ptr->name2 = get_random_ego(INVEN_MAIN_HAND, false);
                    if (this->o_ptr->name2 == EgoType::WEIRD && this->o_ptr->tval != ItemKindType::SWORD) {
                        continue;
                    }

                    auto *e_ptr = &e_info[this->o_ptr->name2];
                    if (this->o_ptr->tval == ItemKindType::SWORD && this->o_ptr->sval == SV_HAYABUSA && e_ptr->max_pval < 0) {
                        if (++n > 1000) {
                            msg_print(_("エラー:隼の剣に割り当てるエゴ無し", "Error: Cannot find for Hayabusa."));
                            return;
                        }

                        continue;
                    }

                    break;
                }
            }
        }

        break;
    }
    case ItemKindType::BOW: {
        if (this->power > 1) {
            if ((this->power > 2) || one_in_(20)) {
                become_random_artifact(this->player_ptr, this->o_ptr, false);
                break;
            }

            this->o_ptr->name2 = get_random_ego(INVEN_BOW, true);
        }

        break;
    }
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT: {
        if (this->power > 1) {
            if (this->power > 2) {
                become_random_artifact(this->player_ptr, this->o_ptr, false);
                break;
            }

            this->o_ptr->name2 = get_random_ego(INVEN_AMMO, true);
            while (one_in_(10L * this->o_ptr->dd * this->o_ptr->ds)) {
                this->o_ptr->dd++;
            }

            if (this->o_ptr->dd > 9) {
                this->o_ptr->dd = 9;
            }

            break;
        }

        if (this->power < -1) {
            if (randint0(MAX_DEPTH) < this->level) {
                this->o_ptr->name2 = get_random_ego(INVEN_AMMO, false);
            }
        }

        break;
    }
    default:
        break;
    }
}
