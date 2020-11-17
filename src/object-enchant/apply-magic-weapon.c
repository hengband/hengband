/*!
 * todo ちょっと長い。要分割
 * @brief 武器系のアイテムを強化して(恐らく床に)生成する処理
 * @date 2020/06/02
 * @author Hourier
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
#include "util/bit-flags-calculator.h"

/*!
 * @brief 武器系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be a "weapon"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special base damage dice boosting\n
 * Hack -- note special processing for weapon/digger\n
 */
void apply_magic_weapon(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
{
    HIT_PROB tohit1 = randint1(5) + (HIT_PROB)m_bonus(5, level);
    HIT_POINT todam1 = randint1(5) + (HIT_POINT)m_bonus(5, level);

    HIT_PROB tohit2 = (HIT_PROB)m_bonus(10, level);
    HIT_POINT todam2 = (HIT_POINT)m_bonus(10, level);

    if ((o_ptr->tval == TV_BOLT) || (o_ptr->tval == TV_ARROW) || (o_ptr->tval == TV_SHOT)) {
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
            o_ptr->curse_flags |= TRC_CURSED;
    }

    if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE))
        return;

    switch (o_ptr->tval) {
    case TV_DIGGING: {
        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(30) || (power > 2))
                become_random_artifact(owner_ptr, o_ptr, FALSE);
            else
                o_ptr->name2 = EGO_DIGGING;
        } else if (power < -1) {
            o_ptr->pval = 0 - (5 + randint1(5));
        } else if (power < 0) {
            o_ptr->pval = 0 - (o_ptr->pval);
        }

        break;
    }
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD: {
        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(40) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }
            while (TRUE) {
                o_ptr->name2 = get_random_ego(INVEN_RARM, TRUE);
                if (o_ptr->name2 == EGO_SHARPNESS && o_ptr->tval != TV_SWORD)
                    continue;
                if (o_ptr->name2 == EGO_EARTHQUAKES && o_ptr->tval != TV_HAFTED)
                    continue;
                if (o_ptr->name2 == EGO_WEIRD && o_ptr->tval != TV_SWORD)
                    continue;
                break;
            }

            switch (o_ptr->name2) {
            case EGO_HA:
                if (one_in_(4) && (level > 40))
                    add_flag(o_ptr->art_flags, TR_BLOWS);
                break;
            case EGO_DF:
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_RES_POIS);
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_WARNING);
                break;
            case EGO_KILL_DRAGON:
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_RES_POIS);
                break;
            case EGO_WEST:
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_RES_FEAR);
                break;
            case EGO_SLAYING_WEAPON:
                if (one_in_(3))
                    o_ptr->dd *= 2;
                else {
                    do {
                        o_ptr->dd++;
                    } while (one_in_(o_ptr->dd));

                    do {
                        o_ptr->ds++;
                    } while (one_in_(o_ptr->ds));
                }

                if (one_in_(5)) {
                    add_flag(o_ptr->art_flags, TR_BRAND_POIS);
                }
                if (o_ptr->tval == TV_SWORD && one_in_(3)) {
                    add_flag(o_ptr->art_flags, TR_VORPAL);
                }
                break;
            case EGO_TRUMP:
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
                if (one_in_(7))
                    one_ability(o_ptr);
                break;
            case EGO_PATTERN:
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_HOLD_EXP);
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_DEX);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_RES_FEAR);
                break;
            case EGO_SHARPNESS:
                o_ptr->pval = (PARAMETER_VALUE)m_bonus(5, level) + 1;
                break;
            case EGO_EARTHQUAKES:
                if (one_in_(3) && (level > 60))
                    add_flag(o_ptr->art_flags, TR_BLOWS);
                else
                    o_ptr->pval = (PARAMETER_VALUE)m_bonus(3, level);
                break;
            case EGO_VAMPIRIC:
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
                break;
            case EGO_DEMON:

                if (one_in_(3))
                    o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
                one_in_(3) ? add_flag(o_ptr->art_flags, TR_DRAIN_EXP)
                           : one_in_(2) ? add_flag(o_ptr->art_flags, TR_DRAIN_HP) : add_flag(o_ptr->art_flags, TR_DRAIN_MANA);

                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_CHAOTIC);
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_BLOWS);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_CALL_DEMON);
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
                while (TRUE) {
                    o_ptr->name2 = get_random_ego(INVEN_RARM, FALSE);
                    if (o_ptr->name2 == EGO_WEIRD && o_ptr->tval != TV_SWORD) {
                        continue;
                    }

                    break;
                }

                switch (o_ptr->name2) {
                case EGO_MORGUL:
                    if (one_in_(6))
                        add_flag(o_ptr->art_flags, TR_TY_CURSE);
                    if (one_in_(3))
                        o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
                    break;
                }
            }
        }

        break;
    }
    case TV_BOW: {
        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }

            o_ptr->name2 = get_random_ego(INVEN_BOW, TRUE);
        }

        break;
    }
    case TV_BOLT:
    case TV_ARROW:
    case TV_SHOT: {
        if (power > 1) {
            /* power > 2 is debug only */
            if (power > 2) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }

            o_ptr->name2 = get_random_ego(INVEN_AMMO, TRUE);
            switch (o_ptr->name2) {
            case EGO_SLAYING_BOLT:
                o_ptr->dd++;
                break;
            }

            while (one_in_(10L * o_ptr->dd * o_ptr->ds))
                o_ptr->dd++;

            if (o_ptr->dd > 9)
                o_ptr->dd = 9;
        } else if (power < -1) {
            if (randint0(MAX_DEPTH) < level) {
                o_ptr->name2 = get_random_ego(INVEN_AMMO, FALSE);
            }
        }

        break;
    }
    }
}
