/*!
 * @brief 防具系のアイテムを強化して(恐らく床に)生成する処理
 * @date 2020/06/02
 * @author Hourier
 * @todo ちょっと長い。要分割
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
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 防具系オブジェクトに生成ランクごとの強化を与えるサブルーチン
 * Apply magic to an item known to be "armor"
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param level 生成基準階
 * @param power 生成ランク
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
            o_ptr->curse_flags.set(TRC::CURSED);
    }

    switch (o_ptr->tval) {
    case TV_DRAG_ARMOR: {
        /* power > 2 is debug only */
        if (one_in_(50) || (power > 2))
            become_random_artifact(owner_ptr, o_ptr, false);
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
                become_random_artifact(owner_ptr, o_ptr, false);
                break;
            }

            while (true) {
                bool okay_flag = true;
                o_ptr->name2 = get_random_ego(INVEN_BODY, true);
                switch (o_ptr->name2) {
                case EGO_DWARVEN:
                    if (o_ptr->tval != TV_HARD_ARMOR) {
                        okay_flag = false;
                    }

                    break;
                case EGO_DRUID:
                    if (o_ptr->tval != TV_SOFT_ARMOR) {
                        okay_flag = false;
                    }

                    break;
                default:
                    break;
                }

                if (okay_flag)
                    break;
            }
        } else if (power < -1) {
            while (true) {
                bool okay_flag = true;
                o_ptr->name2 = get_random_ego(INVEN_BODY, false);

                switch (o_ptr->name2) {
                case EGO_A_DEMON:
                case EGO_A_MORGUL:
                    break;
                default:
                    msg_print(_("エラー：適した呪い鎧エゴがみつかりませんでした.", "Error:Suitable cursed armor ego not found."));
                    okay_flag = true;
                    break;
                }

                if (okay_flag)
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
                become_random_artifact(owner_ptr, o_ptr, false);
                break;
            }

            while (true) {
                o_ptr->name2 = get_random_ego(INVEN_SUB_HAND, true);
                if (o_ptr->sval != SV_SMALL_METAL_SHIELD && o_ptr->sval != SV_LARGE_METAL_SHIELD && o_ptr->name2 == EGO_S_DWARVEN) {
                    continue;
                }

                break;
            }

            switch (o_ptr->name2) {
            case EGO_REFLECTION:
                if (o_ptr->sval == SV_MIRROR_SHIELD)
                    o_ptr->name2 = 0;
                break;
            default:
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
                become_random_artifact(owner_ptr, o_ptr, false);
                break;
            }
            o_ptr->name2 = get_random_ego(INVEN_ARMS, true);
        } else if (power < -1) {
            o_ptr->name2 = get_random_ego(INVEN_ARMS, false);
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
                become_random_artifact(owner_ptr, o_ptr, false);
                break;
            }

            o_ptr->name2 = get_random_ego(INVEN_FEET, true);
        } else if (power < -1) {
            o_ptr->name2 = get_random_ego(INVEN_FEET, false);
        }

        break;
    }
    case TV_CROWN: {
        if (power > 1) {
            /* power > 2 is debug only */
            if (one_in_(20) || (power > 2)) {
                become_random_artifact(owner_ptr, o_ptr, false);
                break;
            }

            while (true) {
                bool ok_flag = true;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, true);

                switch (o_ptr->name2) {
                case EGO_TELEPATHY:
                case EGO_MAGI:
                case EGO_MIGHT:
                case EGO_REGENERATION:
                case EGO_LORDLINESS:
                case EGO_BASILISK:
                    break;
                case EGO_SEEING:
                    if (one_in_(3))
                        add_low_telepathy(o_ptr);
                    break;
                default:
                    /* not existing crown (wisdom,lite, etc...) */
                    ok_flag = false;
                }

                if (ok_flag)
                    break;
            }

            break;
        } else if (power < -1) {
            while (true) {
                bool ok_flag = true;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, false);

                switch (o_ptr->name2) {
                case EGO_H_DEMON:
                    ok_flag = false;
                    break;
                default:
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
                become_random_artifact(owner_ptr, o_ptr, false);
                break;
            }

            while (true) {
                bool ok_flag = true;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, true);
                switch (o_ptr->name2) {
                case EGO_BRILLIANCE:
                case EGO_DARK:  
                case EGO_INFRAVISION:
                case EGO_H_PROTECTION:
                case EGO_LITE:
                    break;
                case EGO_SEEING:
                    if (one_in_(7))
                        add_low_telepathy(o_ptr);
                    break;
                default:
                    /* not existing helm (Magi, Might, etc...)*/
                    ok_flag = false;
                }

                if (ok_flag)
                    break;
            }

            break;
        } else if (power < -1) {
            while (true) {
                bool ok_flag = true;
                o_ptr->name2 = get_random_ego(INVEN_HEAD, false);

                switch (o_ptr->name2) {
                case EGO_ANCIENT_CURSE:
                    ok_flag = false;
                    break;
                default:
                    break;
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
                become_random_artifact(owner_ptr, o_ptr, false);
                break;
            }
            o_ptr->name2 = get_random_ego(INVEN_OUTER, true);
        } else if (power < -1) {
            o_ptr->name2 = get_random_ego(INVEN_OUTER, false);
        }

        break;
    }

    default:
        break;
    }
}
