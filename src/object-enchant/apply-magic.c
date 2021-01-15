/*!
 * todo 少し長い。要分割
 * @brief ベースアイテムを強化する処理
 * @date 2020/06/03
 * @author Hourier
 */

#include "object-enchant/apply-magic.h"
#include "artifact/fixed-art-types.h"
#include "artifact/fixed-art-generator.h"
#include "dungeon/dungeon.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/apply-magic-accessory.h"
#include "object-enchant/apply-magic-armor.h"
#include "object-enchant/apply-magic-others.h"
#include "object-enchant/apply-magic-weapon.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-kind.h"
#include "player/player-status-flags.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*!
 * @brief 生成されたベースアイテムに魔法的な強化を与えるメインルーチン
 * Complete the "creation" of an object by applying "magic" to the item
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param lev 生成基準階
 * @param mode 生成オプション
 * @return なし
 * @details
 * エゴ＆アーティファクトの生成、呪い、pval強化
 */
void apply_magic(player_type *owner_ptr, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode)
{
    if (owner_ptr->pseikaku == PERSONALITY_MUNCHKIN)
        lev += randint0(owner_ptr->lev / 2 + 10);
    if (lev > MAX_DEPTH - 1)
        lev = MAX_DEPTH - 1;

    int f1 = lev + 10;
    if (f1 > d_info[owner_ptr->dungeon_idx].obj_good)
        f1 = d_info[owner_ptr->dungeon_idx].obj_good;

    int f2 = f1 * 2 / 3;
    if ((owner_ptr->pseikaku != PERSONALITY_MUNCHKIN) && (f2 > d_info[owner_ptr->dungeon_idx].obj_great))
        f2 = d_info[owner_ptr->dungeon_idx].obj_great;

    if (has_good_luck(owner_ptr)) {
        f1 += 5;
        f2 += 2;
    } else if (owner_ptr->muta3 & MUT3_BAD_LUCK) {
        f1 -= 5;
        f2 -= 2;
    }

    int power = 0;
    if ((mode & AM_GOOD) || magik(f1)) {
        power = 1;
        if ((mode & AM_GREAT) || magik(f2)) {
            power = 2;
            if (mode & AM_SPECIAL)
                power = 3;
        }
    } else if (magik(f1)) {
        power = -1;
        if (magik(f2))
            power = -2;
    }
    if (mode & AM_CURSED) {
        if (power > 0) {
            power = 0 - power;
        } else {
            power--;
        }
    }

    int rolls = 0;
    if (power >= 2)
        rolls = 1;

    if (mode & (AM_GREAT | AM_SPECIAL))
        rolls = 4;
    if ((mode & AM_NO_FIXED_ART) || o_ptr->name1)
        rolls = 0;

    for (int i = 0; i < rolls; i++) {
        if (make_artifact(owner_ptr, o_ptr))
            break;
        if (has_good_luck(owner_ptr) && one_in_(77)) {
            if (make_artifact(owner_ptr, o_ptr))
                break;
        }
    }

    if (object_is_fixed_artifact(o_ptr)) {
        artifact_type *a_ptr = &a_info[o_ptr->name1];
        a_ptr->cur_num = 1;
        if (current_world_ptr->character_dungeon)
            a_ptr->floor_id = owner_ptr->floor_id;

        o_ptr->pval = a_ptr->pval;
        o_ptr->ac = a_ptr->ac;
        o_ptr->dd = a_ptr->dd;
        o_ptr->ds = a_ptr->ds;
        o_ptr->to_a = a_ptr->to_a;
        o_ptr->to_h = a_ptr->to_h;
        o_ptr->to_d = a_ptr->to_d;
        o_ptr->weight = a_ptr->weight;
        o_ptr->xtra2 = a_ptr->act_idx;

        if (o_ptr->name1 == ART_MILIM) {
            if (owner_ptr->pseikaku == PERSONALITY_SEXY) {
                o_ptr->pval = 3;
            }
        }

        if (!a_ptr->cost)
            o_ptr->ident |= (IDENT_BROKEN);
        if (a_ptr->gen_flags & TRG_CURSED)
            o_ptr->curse_flags |= (TRC_CURSED);
        if (a_ptr->gen_flags & TRG_HEAVY_CURSE)
            o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
        if (a_ptr->gen_flags & TRG_PERMA_CURSE)
            o_ptr->curse_flags |= (TRC_PERMA_CURSE);
        if (a_ptr->gen_flags & (TRG_RANDOM_CURSE0))
            o_ptr->curse_flags |= get_curse(owner_ptr, 0, o_ptr);
        if (a_ptr->gen_flags & (TRG_RANDOM_CURSE1))
            o_ptr->curse_flags |= get_curse(owner_ptr, 1, o_ptr);
        if (a_ptr->gen_flags & (TRG_RANDOM_CURSE2))
            o_ptr->curse_flags |= get_curse(owner_ptr, 2, o_ptr);

        return;
    }

    switch (o_ptr->tval) {
    case TV_DIGGING:
    case TV_HAFTED:
    case TV_BOW:
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT: {
        if (power)
            apply_magic_weapon(owner_ptr, o_ptr, lev, power);
        break;
    }
    case TV_POLEARM: {
        if (power && !(o_ptr->sval == SV_DEATH_SCYTHE))
            apply_magic_weapon(owner_ptr, o_ptr, lev, power);
        break;
    }
    case TV_SWORD: {
        if (power && !(o_ptr->sval == SV_POISON_NEEDLE))
            apply_magic_weapon(owner_ptr, o_ptr, lev, power);
        break;
    }
    case TV_DRAG_ARMOR:
    case TV_HARD_ARMOR:
    case TV_SOFT_ARMOR:
    case TV_SHIELD:
    case TV_HELM:
    case TV_CROWN:
    case TV_CLOAK:
    case TV_GLOVES:
    case TV_BOOTS: {
        if (((o_ptr->tval == TV_CLOAK) && (o_ptr->sval == SV_ELVEN_CLOAK)) || ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_KUROSHOUZOKU)))
            o_ptr->pval = randint1(4);

        if (power || ((o_ptr->tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM)) || ((o_ptr->tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD))
            || ((o_ptr->tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES))
            || ((o_ptr->tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)))
            apply_magic_armor(owner_ptr, o_ptr, lev, power);

        break;
    }
    case TV_RING:
    case TV_AMULET: {
        if (!power && (randint0(100) < 50))
            power = -1;
        apply_magic_accessary(owner_ptr, o_ptr, lev, power);
        break;
    }
    default: {
        apply_magic_others(owner_ptr, o_ptr, power);
        break;
    }
    }

    if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI) && (owner_ptr->pseikaku == PERSONALITY_SEXY)) {
        o_ptr->pval = 3;
        add_flag(o_ptr->art_flags, TR_STR);
        add_flag(o_ptr->art_flags, TR_INT);
        add_flag(o_ptr->art_flags, TR_WIS);
        add_flag(o_ptr->art_flags, TR_DEX);
        add_flag(o_ptr->art_flags, TR_CON);
        add_flag(o_ptr->art_flags, TR_CHR);
    }

    if (object_is_ego(o_ptr)) {
        ego_item_type *e_ptr = &e_info[o_ptr->name2];
        if (!e_ptr->cost)
            o_ptr->ident |= (IDENT_BROKEN);

        if (e_ptr->gen_flags & TRG_CURSED)
            o_ptr->curse_flags |= (TRC_CURSED);
        if (e_ptr->gen_flags & TRG_HEAVY_CURSE)
            o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
        if (e_ptr->gen_flags & TRG_PERMA_CURSE)
            o_ptr->curse_flags |= (TRC_PERMA_CURSE);
        if (e_ptr->gen_flags & (TRG_RANDOM_CURSE0))
            o_ptr->curse_flags |= get_curse(owner_ptr, 0, o_ptr);
        if (e_ptr->gen_flags & (TRG_RANDOM_CURSE1))
            o_ptr->curse_flags |= get_curse(owner_ptr, 1, o_ptr);
        if (e_ptr->gen_flags & (TRG_RANDOM_CURSE2))
            o_ptr->curse_flags |= get_curse(owner_ptr, 2, o_ptr);

        if (e_ptr->gen_flags & (TRG_ONE_SUSTAIN))
            one_sustain(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_POWER))
            one_ability(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_H_RES))
            one_high_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_E_RES))
            one_ele_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_D_RES))
            one_dragon_ele_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_L_RES))
            one_lordly_high_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_RES))
            one_resistance(o_ptr);
        if (e_ptr->gen_flags & (TRG_XTRA_DICE)) {
            do {
                o_ptr->dd++;
            } while (one_in_(o_ptr->dd));

            if (o_ptr->dd > 9)
                o_ptr->dd = 9;
        }

        if (e_ptr->act_idx)
            o_ptr->xtra2 = (XTRA8)e_ptr->act_idx;

        if ((object_is_cursed(o_ptr) || object_is_broken(o_ptr)) && !(e_ptr->gen_flags & (TRG_POWERFUL))) {
            if (e_ptr->max_to_h)
                o_ptr->to_h -= randint1(e_ptr->max_to_h);
            if (e_ptr->max_to_d)
                o_ptr->to_d -= randint1(e_ptr->max_to_d);
            if (e_ptr->max_to_a)
                o_ptr->to_a -= randint1(e_ptr->max_to_a);
            if (e_ptr->max_pval)
                o_ptr->pval -= randint1(e_ptr->max_pval);
        } else {
            if (e_ptr->max_to_h) {
                if (e_ptr->max_to_h > 127)
                    o_ptr->to_h -= randint1(256 - e_ptr->max_to_h);
                else
                    o_ptr->to_h += randint1(e_ptr->max_to_h);
            }

            if (e_ptr->max_to_d) {
                if (e_ptr->max_to_d > 127)
                    o_ptr->to_d -= randint1(256 - e_ptr->max_to_d);
                else
                    o_ptr->to_d += randint1(e_ptr->max_to_d);
            }

            if (e_ptr->max_to_a) {
                if (e_ptr->max_to_a > 127)
                    o_ptr->to_a -= randint1(256 - e_ptr->max_to_a);
                else
                    o_ptr->to_a += randint1(e_ptr->max_to_a);
            }

            if (o_ptr->name2 == EGO_ACCURACY) {
                while (o_ptr->to_h < o_ptr->to_d + 10) {
                    o_ptr->to_h += 5;
                    o_ptr->to_d -= 5;
                }
                o_ptr->to_h = MAX(o_ptr->to_h, 15);
            }

            if (o_ptr->name2 == EGO_VELOCITY) {
                while (o_ptr->to_d < o_ptr->to_h + 10) {
                    o_ptr->to_d += 5;
                    o_ptr->to_h -= 5;
                }
                o_ptr->to_d = MAX(o_ptr->to_d, 15);
            }

            if ((o_ptr->name2 == EGO_PROTECTION) || (o_ptr->name2 == EGO_S_PROTECTION) || (o_ptr->name2 == EGO_H_PROTECTION)) {
                o_ptr->to_a = MAX(o_ptr->to_a, 15);
            }

            if (e_ptr->max_pval) {
                if ((o_ptr->name2 == EGO_HA) && (has_flag(o_ptr->art_flags, TR_BLOWS))) {
                    o_ptr->pval++;
                    if ((lev > 60) && one_in_(3) && ((o_ptr->dd * (o_ptr->ds + 1)) < 15))
                        o_ptr->pval++;
                } else if (o_ptr->name2 == EGO_DEMON) {
                    if (has_flag(o_ptr->art_flags, TR_BLOWS)) {
                        o_ptr->pval += randint1(2);
                    } else {
                        o_ptr->pval += randint1(e_ptr->max_pval);
                    }
                } else if (o_ptr->name2 == EGO_ATTACKS) {
                    o_ptr->pval = randint1(e_ptr->max_pval * lev / 100 + 1);
                    if (o_ptr->pval > 3)
                        o_ptr->pval = 3;
                    if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
                        o_ptr->pval += randint1(2);
                } else if (o_ptr->name2 == EGO_BAT) {
                    o_ptr->pval = randint1(e_ptr->max_pval);
                    if (o_ptr->sval == SV_ELVEN_CLOAK)
                        o_ptr->pval += randint1(2);
                } else if (o_ptr->name2 == EGO_A_DEMON || o_ptr->name2 == EGO_DRUID || o_ptr->name2 == EGO_OLOG) {
                    o_ptr->pval = randint1(e_ptr->max_pval);
                } else {
                    o_ptr->pval += randint1(e_ptr->max_pval);
                }
            }

            if ((o_ptr->name2 == EGO_SPEED) && (lev < 50)) {
                o_ptr->pval = randint1(o_ptr->pval);
            }

            if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA) && (o_ptr->pval > 2) && (o_ptr->name2 != EGO_ATTACKS))
                o_ptr->pval = 2;
        }

        return;
    }

    if (o_ptr->k_idx) {
        object_kind *k_ptr = &k_info[o_ptr->k_idx];
        if (!k_info[o_ptr->k_idx].cost)
            o_ptr->ident |= (IDENT_BROKEN);

        if (k_ptr->gen_flags & (TRG_CURSED))
            o_ptr->curse_flags |= (TRC_CURSED);
        if (k_ptr->gen_flags & (TRG_HEAVY_CURSE))
            o_ptr->curse_flags |= TRC_HEAVY_CURSE;
        if (k_ptr->gen_flags & (TRG_PERMA_CURSE))
            o_ptr->curse_flags |= TRC_PERMA_CURSE;
        if (k_ptr->gen_flags & (TRG_RANDOM_CURSE0))
            o_ptr->curse_flags |= get_curse(owner_ptr, 0, o_ptr);
        if (k_ptr->gen_flags & (TRG_RANDOM_CURSE1))
            o_ptr->curse_flags |= get_curse(owner_ptr, 1, o_ptr);
        if (k_ptr->gen_flags & (TRG_RANDOM_CURSE2))
            o_ptr->curse_flags |= get_curse(owner_ptr, 2, o_ptr);
    }
}
