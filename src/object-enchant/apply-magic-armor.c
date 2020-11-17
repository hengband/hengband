/*!
 * todo ちょっと長い。要分割
 * @brief 防具系のアイテムを強化して(恐らく床に)生成する処理
 * @date 2020/06/02
 * @author Hourier
 */

#include "object-enchant/apply-magic-armor.h"
#include "artifact/random-art-generator.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-ego.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 防具系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be "armor"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
 * @return なし
 * @details
 * Hack -- note special processing for crown/helm\n
 * Hack -- note special processing for robe of permanence\n
 */
void apply_magic_armor(player_type *owner_ptr, object_type *o_ptr, DEPTH level, int power)
{
    ARMOUR_CLASS toac1 = (ARMOUR_CLASS)randint1(5) + m_bonus(5, level);
    ARMOUR_CLASS toac2 = (ARMOUR_CLASS)m_bonus(10, level);
    if (power > 0) {
        o_ptr->to_a += toac1;
        if (power > 1) {
            o_ptr->to_a += toac2;
        }
    } else if (power < 0) {
        o_ptr->to_a -= toac1;
        if (power < -1) {
            o_ptr->to_a -= toac2;
        }

        if (o_ptr->to_a < 0)
            o_ptr->curse_flags |= TRC_CURSED;
    }

    switch (o_ptr->tval) {
    case TV_DRAG_ARMOR: {
        /* power > 2 is debug only */
        if (one_in_(50) || (power > 2))
            become_random_artifact(owner_ptr, o_ptr, FALSE);
        break;
    }
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR: {
        if (power > 1) {
            if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ROBE) && (randint0(100) < 15)) {
                if (one_in_(5)) {
                    o_ptr->name2 = EGO_YOIYAMI;
                    o_ptr->k_idx = lookup_kind(TV_SOFT_ARMOR, SV_YOIYAMI_ROBE);
                    o_ptr->sval = SV_YOIYAMI_ROBE;
                    o_ptr->ac = 0;
                    o_ptr->to_a = 0;
                } else {
                    o_ptr->name2 = EGO_PERMANENCE;
                }

                break;
            }

            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }

            while (TRUE) {
                bool okay_flag = TRUE;
                o_ptr->name2 = get_random_ego(INVEN_BODY, TRUE);
                switch (o_ptr->name2) {
                case EGO_DWARVEN:
                    if (o_ptr->tval != TV_HARD_ARMOR) {
                        okay_flag = FALSE;
                    }

                    break;
                case EGO_DRUID:
                    if (o_ptr->tval != TV_SOFT_ARMOR) {
                        okay_flag = FALSE;
                    }

                    break;
                default:
                    break;
                }

                if (okay_flag)
                    break;
            }

            switch (o_ptr->name2) {
            case EGO_RESISTANCE:
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_RES_POIS);
                break;
            case EGO_DWARVEN:
                o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
                o_ptr->ac = k_info[o_ptr->k_idx].ac + 5;
                break;

            case EGO_A_DEMON:
                if (one_in_(3))
                    o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
                one_in_(3) ? add_flag(o_ptr->art_flags, TR_DRAIN_EXP)
                           : one_in_(2) ? add_flag(o_ptr->art_flags, TR_DRAIN_HP) : add_flag(o_ptr->art_flags, TR_DRAIN_MANA);

                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_AGGRAVATE);
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_ADD_L_CURSE);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_DRAIN_HP);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_DRAIN_MANA);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_TY_CURSE);
                if (one_in_(5))
                    add_flag(o_ptr->art_flags, TR_CALL_DEMON);
                break;
            case EGO_A_MORGUL:
                if (one_in_(3))
                    o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
                if (one_in_(9))
                    add_flag(o_ptr->art_flags, TR_TY_CURSE);
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
                if (one_in_(6))
                    add_flag(o_ptr->art_flags, TR_AGGRAVATE);
                if (one_in_(9))
                    add_flag(o_ptr->art_flags, TR_NO_MAGIC);
                if (one_in_(9))
                    add_flag(o_ptr->art_flags, TR_NO_TELE);
                break;
            default:
                break;
            }
        }

        break;
    }
    case TV_SHIELD: {
        if (o_ptr->sval == SV_DRAGON_SHIELD) {
            dragon_resist(o_ptr);
            if (!one_in_(3))
                break;
        }

        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }

            while (TRUE) {
                o_ptr->name2 = get_random_ego(INVEN_LARM, TRUE);
                if (o_ptr->sval != SV_SMALL_METAL_SHIELD && o_ptr->sval != SV_LARGE_METAL_SHIELD && o_ptr->name2 == EGO_S_DWARVEN) {
                    continue;
                }

                break;
            }

            switch (o_ptr->name2) {
            case EGO_ENDURANCE:
                if (!one_in_(3))
                    one_high_resistance(o_ptr);
                if (one_in_(4))
                    add_flag(o_ptr->art_flags, TR_RES_POIS);
                break;
            case EGO_REFLECTION:
                if (o_ptr->sval == SV_MIRROR_SHIELD)
                    o_ptr->name2 = 0;
                break;

            case EGO_S_DWARVEN:
                o_ptr->weight = (2 * k_info[o_ptr->k_idx].weight / 3);
                o_ptr->ac = k_info[o_ptr->k_idx].ac + 3;
                break;
            }
        }

        break;
    }
    case TV_GLOVES: {
        if (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES) {
            dragon_resist(o_ptr);
            if (!one_in_(3))
                break;
        }

        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }
            o_ptr->name2 = get_random_ego(INVEN_HANDS, TRUE);
        } else if (power < -1) {
            o_ptr->name2 = get_random_ego(INVEN_HANDS, FALSE);
        }

        break;
    }

    case TV_BOOTS: {
        if (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE) {
            dragon_resist(o_ptr);
            if (!one_in_(3))
                break;
        }

        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }

            o_ptr->name2 = get_random_ego(INVEN_FEET, TRUE);
            switch (o_ptr->name2) {
            case EGO_SLOW_DESCENT:
                if (one_in_(2)) {
                    one_high_resistance(o_ptr);
                }

                break;
            }
        } else if (power < -1) {
            o_ptr->name2 = get_random_ego(INVEN_FEET, FALSE);
        }

        break;
    }
    case TV_CROWN: {
        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }

            while (TRUE) {
                bool ok_flag = TRUE;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, TRUE);

                switch (o_ptr->name2) {
                case EGO_TELEPATHY:
                    if (add_esp_strong(o_ptr))
                        add_esp_weak(o_ptr, TRUE);
                    else
                        add_esp_weak(o_ptr, FALSE);
                    break;
                case EGO_MAGI:
                case EGO_MIGHT:
                case EGO_REGENERATION:
                case EGO_LORDLINESS:
                case EGO_BASILISK:
                    break;
                case EGO_SEEING:
                    if (one_in_(3)) {
                        if (one_in_(2))
                            add_esp_strong(o_ptr);
                        else
                            add_esp_weak(o_ptr, FALSE);
                    }
                    break;
                default:
                    /* not existing crown (wisdom,lite, etc...) */
                    ok_flag = FALSE;
                }

                if (ok_flag)
                    break;
            }

            break;
        } else if (power < -1) {
            while (TRUE) {
                bool ok_flag = TRUE;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, FALSE);

                switch (o_ptr->name2) {
                case EGO_ANCIENT_CURSE:
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_NO_MAGIC);
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_NO_TELE);
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_TY_CURSE);
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_DRAIN_HP);
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_DRAIN_MANA);
                    break;
                }

                if (ok_flag)
                    break;
            }
        }

        break;
    }
    case TV_HELM: {
        if (o_ptr->sval == SV_DRAGON_HELM) {
            dragon_resist(o_ptr);
            if (!one_in_(3))
                break;
        }

        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }

            while (TRUE) {
                bool ok_flag = TRUE;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, TRUE);
                switch (o_ptr->name2) {
                case EGO_BRILLIANCE:
                case EGO_DARK:
                case EGO_INFRAVISION:
                case EGO_H_PROTECTION:
                    break;
                case EGO_SEEING:
                    if (one_in_(7)) {
                        if (one_in_(2))
                            add_esp_strong(o_ptr);
                        else
                            add_esp_weak(o_ptr, FALSE);
                    }

                    break;
                case EGO_LITE:
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_LITE_1);
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_LITE_2);
                    break;
                case EGO_H_DEMON:
                    if (one_in_(3))
                        o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
                    one_in_(3) ? add_flag(o_ptr->art_flags, TR_DRAIN_EXP)
                               : one_in_(2) ? add_flag(o_ptr->art_flags, TR_DRAIN_HP) : add_flag(o_ptr->art_flags, TR_DRAIN_MANA);

                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_AGGRAVATE);
                    if (one_in_(3))
                        add_flag(o_ptr->art_flags, TR_ADD_L_CURSE);
                    if (one_in_(5))
                        add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
                    if (one_in_(5))
                        add_flag(o_ptr->art_flags, TR_DRAIN_HP);
                    if (one_in_(5))
                        add_flag(o_ptr->art_flags, TR_DRAIN_MANA);
                    if (one_in_(5))
                        add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
                    if (one_in_(5))
                        add_flag(o_ptr->art_flags, TR_TY_CURSE);
                    if (one_in_(5))
                        add_flag(o_ptr->art_flags, TR_CALL_DEMON);
                    break;
                default:
                    /* not existing helm (Magi, Might, etc...)*/
                    ok_flag = FALSE;
                }
                if (ok_flag)
                    break;
            }

            break;
        } else if (power < -1) {
            while (TRUE) {
                bool ok_flag = TRUE;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, FALSE);

                switch (o_ptr->name2) {
                case EGO_ANCIENT_CURSE:
                    ok_flag = FALSE;
                }

                if (ok_flag)
                    break;
            }
        }

        break;
    }
    case TV_CLOAK: {
        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, FALSE);
                break;
            }
            o_ptr->name2 = get_random_ego(INVEN_OUTER, TRUE);

            switch (o_ptr->name2) {
            case EGO_BAT:
                o_ptr->to_d -= 6;
                o_ptr->to_h -= 6;
                break;
            case EGO_NAZGUL:
                o_ptr->to_d -= 3;
                o_ptr->to_h -= 3;
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_COWARDICE);
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_CALL_UNDEAD);
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_SLOW_REGEN);
                if (one_in_(3))
                    add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
                break;
            }

        } else if (power < -1) {
            o_ptr->name2 = get_random_ego(INVEN_OUTER, FALSE);
        }

        break;
    }
    }
}
