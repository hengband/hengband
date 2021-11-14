/*!
 * @brief 武器系のアイテムを強化して(恐らく床に)生成する処理
 * @date 2020/06/02
 * @author Hourier
 * @todo ちょっと長い。要分割
 */

#include "object-enchant/apply-magic-weapon.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 武器系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "weapon"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @details
 * Hack -- note special base damage dice boosting\n
 * Hack -- note special processing for weapon/digger\n
 */
void apply_magic_weapon(PlayerType *player_ptr, object_type *o_ptr, DEPTH level, int power)
{
    HIT_PROB tohit1 = randint1(5) + (HIT_PROB)m_bonus(5, level);
    HIT_POINT todam1 = randint1(5) + (HIT_POINT)m_bonus(5, level);

    HIT_PROB tohit2 = (HIT_PROB)m_bonus(10, level);
    HIT_POINT todam2 = (HIT_POINT)m_bonus(10, level);

    if ((o_ptr->tval == ItemKindType::BOLT) || (o_ptr->tval == ItemKindType::ARROW) || (o_ptr->tval == ItemKindType::SHOT)) {
        tohit2 = (tohit2 + 1) / 2;
        todam2 = (todam2 + 1) / 2;
    }

    if (power > 0) {
        o_ptr->to_h += tohit1;
        o_ptr->to_d += todam1;
        if (power > 1) {
            o_ptr->to_h += tohit2;
            o_ptr->to_d += todam2;
        }
    } else if (power < 0) {
        o_ptr->to_h -= tohit1;
        o_ptr->to_d -= todam1;
        if (power < -1) {
            o_ptr->to_h -= tohit2;
            o_ptr->to_d -= todam2;
        }

        if (o_ptr->to_h + o_ptr->to_d < 0)
            o_ptr->curse_flags.set(CurseTraitType::CURSED);
    }

    if ((o_ptr->tval == ItemKindType::SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE))
        return;

    switch (o_ptr->tval) {
    case ItemKindType::DIGGING: {
        if (power > 1) {
            /* power > 2はデバッグ専用. */
            if (one_in_(30) || (power > 2))
                become_random_artifact(player_ptr, o_ptr, false);
            else
                o_ptr->name2 = EGO_DIGGING;
        } else if (power < -1) {
            o_ptr->pval = 0 - (5 + randint1(5));
        } else if (power < 0) {
            o_ptr->pval = 0 - (o_ptr->pval);
        }

        break;
    }
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD: {
        if (power > 1) {
            /* power > 2はデバッグ専用. */
            if (one_in_(40) || (power > 2)) {
                become_random_artifact(player_ptr, o_ptr, false);
                break;
            }
            while (true) {
                o_ptr->name2 = get_random_ego(INVEN_MAIN_HAND, true);
                if (o_ptr->name2 == EGO_SHARPNESS && o_ptr->tval != ItemKindType::SWORD)
                    continue;
                if (o_ptr->name2 == EGO_EARTHQUAKES && o_ptr->tval != ItemKindType::HAFTED)
                    continue;
                break;
            }

            switch (o_ptr->name2) {
            case EGO_SHARPNESS:
                o_ptr->pval = (PARAMETER_VALUE)m_bonus(5, level) + 1;
                break;
            case EGO_EARTHQUAKES:
                if (one_in_(3) && (level > 60))
                    o_ptr->art_flags.set(TR_BLOWS);
                else
                    o_ptr->pval = (PARAMETER_VALUE)m_bonus(3, level);
                break;
            default:
                break;
            }

            if (!o_ptr->art_name) {
                while (one_in_(10L * o_ptr->dd * o_ptr->ds))
                    o_ptr->dd++;

                if (o_ptr->dd > 9)
                    o_ptr->dd = 9;
            }
        } else if (power < -1) {
            if (randint0(MAX_DEPTH) < level) {
                int n = 0;
                while (true) {
                    o_ptr->name2 = get_random_ego(INVEN_MAIN_HAND, false);
                    if (o_ptr->name2 == EGO_WEIRD && o_ptr->tval != ItemKindType::SWORD) {
                        continue;
                    }

                    ego_item_type *e_ptr = &e_info[o_ptr->name2];
                    if (o_ptr->tval == ItemKindType::SWORD && o_ptr->sval == SV_HAYABUSA && e_ptr->max_pval < 0) {
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
        if (power > 1) {
            /* power > 2はデバッグ専用. */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(player_ptr, o_ptr, false);
                break;
            }

            o_ptr->name2 = get_random_ego(INVEN_BOW, true);
        }

        break;
    }
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT: {
        if (power > 1) {
            /* power > 2はデバッグ専用. */
            if (power > 2) {
                become_random_artifact(player_ptr, o_ptr, false);
                break;
            }

            o_ptr->name2 = get_random_ego(INVEN_AMMO, true);

            while (one_in_(10L * o_ptr->dd * o_ptr->ds))
                o_ptr->dd++;

            if (o_ptr->dd > 9)
                o_ptr->dd = 9;
        } else if (power < -1) {
            if (randint0(MAX_DEPTH) < level) {
                o_ptr->name2 = get_random_ego(INVEN_AMMO, false);
            }
        }

        break;
    }

    default:
        break;
    }
}
