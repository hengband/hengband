/*!
 * @brief 固定アーティファクトの生成 / Artifact code
 * @date 2020/07/14
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 * 2013 Deskull rearranged comment for Doxygen.
 * 2020 Hourier rearranged
 */

#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "floor/floor-object.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "specific-object/bloody-moon.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 恐怖の仮面への特殊処理 (一部職業のみ追加能力＆耐性、それ以外は反感＆太古の怨念)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体への参照ポインタ
 * @param give_power 追加能力の有無
 * @param give_resistance 追加耐性の有無
 * @return そもそも対象のオブジェクトが恐怖の仮面ではないか、「その他の職業」であればTRUE
 */
static bool invest_terror_mask(player_type *player_ptr, object_type *o_ptr, bool *give_power, bool *give_resistance)
{
    if (o_ptr->name1 != ART_TERROR)
        return FALSE;

    bool is_special_class = player_ptr->pclass == CLASS_WARRIOR;
    is_special_class |= player_ptr->pclass == CLASS_ARCHER;
    is_special_class |= player_ptr->pclass == CLASS_CAVALRY;
    is_special_class |= player_ptr->pclass == CLASS_BERSERKER;
    if (is_special_class) {
        *give_power = TRUE;
        *give_resistance = TRUE;
        return FALSE;
    }

    add_flag(o_ptr->art_flags, TR_AGGRAVATE);
    add_flag(o_ptr->art_flags, TR_TY_CURSE);
    o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
    o_ptr->curse_flags |= get_curse(player_ptr, 2, o_ptr);
    return TRUE;
}

/*!
 * @brief 戦乙女ミリムの危ない水着への特殊処理 (セクシーギャルのみpval追加)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体への参照ポインタ
 * @return なし
 */
static void milim_swimsuit(player_type *player_ptr, object_type *o_ptr)
{
    if ((o_ptr->name1 != ART_MILIM) || (player_ptr->pseikaku != PERSONALITY_SEXY))
        return;

    o_ptr->pval = 3;
    add_flag(o_ptr->art_flags, TR_STR);
    add_flag(o_ptr->art_flags, TR_INT);
    add_flag(o_ptr->art_flags, TR_WIS);
    add_flag(o_ptr->art_flags, TR_DEX);
    add_flag(o_ptr->art_flags, TR_CON);
    add_flag(o_ptr->art_flags, TR_CHR);
}

/*!
 * @brief 固定アーティファクト生成時の特別なハードコーディング処理を行う。.
 * @details random_artifact_resistance()とあるが実際は固定アーティファクトである。
 * 対象は恐怖の仮面、村正、ロビントンのハープ、龍争虎鬪、ブラッディムーン、羽衣、天女の羽衣、ミリム、
 * その他追加耐性、特性追加処理。
 * @attention プレイヤーの各種ステータスに依存した処理がある。
 * @todo 折を見て関数名を変更すること。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_ptr 生成する固定アーティファクト構造体ポインタ
 * @return なし
 */
static void random_artifact_resistance(player_type *player_ptr, object_type *o_ptr, artifact_type *a_ptr)
{
    bool give_power = FALSE;
    bool give_resistance = FALSE;
    if (invest_terror_mask(player_ptr, o_ptr, &give_power, &give_resistance))
        return;

    if ((o_ptr->name1 == ART_MURAMASA) && (player_ptr->pclass != CLASS_SAMURAI)) {
        add_flag(o_ptr->art_flags, TR_NO_MAGIC);
        o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
    }

    if ((o_ptr->name1 == ART_ROBINTON) && (player_ptr->pclass == CLASS_BARD))
        add_flag(o_ptr->art_flags, TR_DEC_MANA);

    if ((o_ptr->name1 == ART_XIAOLONG) && (player_ptr->pclass == CLASS_MONK))
        add_flag(o_ptr->art_flags, TR_BLOWS);

    if (o_ptr->name1 == ART_BLOOD)
        get_bloody_moon_flags(o_ptr);

    if ((o_ptr->name1 == ART_HEAVENLY_MAIDEN) && (player_ptr->psex != SEX_FEMALE))
        add_flag(o_ptr->art_flags, TR_AGGRAVATE);

    milim_swimsuit(player_ptr, o_ptr);
    if (a_ptr->gen_flags & TRG_XTRA_POWER)
        give_power = TRUE;

    if (a_ptr->gen_flags & TRG_XTRA_H_RES)
        give_resistance = TRUE;

    if (a_ptr->gen_flags & TRG_XTRA_RES_OR_POWER) {
        if (one_in_(2))
            give_resistance = TRUE;
        else
            give_power = TRUE;
    }

    if (give_power)
        one_ability(o_ptr);

    if (give_resistance)
        one_high_resistance(o_ptr);
}

static void invest_curse_to_fixed_artifact(player_type *player_ptr, artifact_type *a_ptr, object_type *q_ptr)
{
    if (a_ptr->gen_flags & TRG_CURSED)
        q_ptr->curse_flags |= TRC_CURSED;

    if (a_ptr->gen_flags & TRG_HEAVY_CURSE)
        q_ptr->curse_flags |= TRC_HEAVY_CURSE;

    if (a_ptr->gen_flags & TRG_PERMA_CURSE)
        q_ptr->curse_flags |= TRC_PERMA_CURSE;

    if (a_ptr->gen_flags & TRG_RANDOM_CURSE0)
        q_ptr->curse_flags |= get_curse(player_ptr, 0, q_ptr);

    if (a_ptr->gen_flags & TRG_RANDOM_CURSE1)
        q_ptr->curse_flags |= get_curse(player_ptr, 1, q_ptr);

    if (a_ptr->gen_flags & TRG_RANDOM_CURSE2)
        q_ptr->curse_flags |= get_curse(player_ptr, 2, q_ptr);
}

/*!
 * @brief フロアの指定された位置に固定アーティファクトを生成する。 / Create the artifact of the specified number
 * @details 固定アーティファクト構造体から基本ステータスをコピーした後、所定の座標でdrop_item()で落とす。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param a_idx 生成する固定アーティファクト構造体のID
 * @param y アイテムを落とす地点のy座標
 * @param x アイテムを落とす地点のx座標
 * @return 生成が成功したか否か、失敗はIDの不全、ベースアイテムの不全、drop_item()の失敗時に起こる。
 * @attention この処理はdrop_near()内で普通の固定アーティファクトが重ならない性質に依存する.
 * 仮に2個以上存在可能かつ装備品以外の固定アーティファクトが作成されれば
 * drop_near()関数の返り値は信用できなくなる.
 */
bool create_named_art(player_type *player_ptr, ARTIFACT_IDX a_idx, POSITION y, POSITION x)
{
    artifact_type *a_ptr = &a_info[a_idx];
    if (!a_ptr->name)
        return FALSE;

    KIND_OBJECT_IDX i = lookup_kind(a_ptr->tval, a_ptr->sval);
    if (i == 0)
        return FALSE;

    object_type forge;
    object_type *q_ptr;
    q_ptr = &forge;
    object_prep(player_ptr, q_ptr, i);
    q_ptr->name1 = a_idx;
    q_ptr->pval = a_ptr->pval;
    q_ptr->ac = a_ptr->ac;
    q_ptr->dd = a_ptr->dd;
    q_ptr->ds = a_ptr->ds;
    q_ptr->to_a = a_ptr->to_a;
    q_ptr->to_h = a_ptr->to_h;
    q_ptr->to_d = a_ptr->to_d;
    q_ptr->weight = a_ptr->weight;
    invest_curse_to_fixed_artifact(player_ptr, a_ptr, q_ptr);
    random_artifact_resistance(player_ptr, q_ptr, a_ptr);
    return drop_near(player_ptr, q_ptr, -1, y, x) ? TRUE : FALSE;
}

/*!
 * @brief 非INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する。
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 生成に割り当てたいオブジェクトの構造体参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * Attempt to change an object into an artifact\n
 * This routine should only be called by "apply_magic()"\n
 * Note -- see "make_artifact_special()" and "apply_magic()"\n
 */
bool make_artifact(player_type *player_ptr, object_type *o_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level == 0)
        return FALSE;

    if (o_ptr->number != 1)
        return FALSE;

    for (ARTIFACT_IDX i = 0; i < max_a_idx; i++) {
        artifact_type *a_ptr = &a_info[i];
        if (!a_ptr->name)
            continue;

        if (a_ptr->cur_num)
            continue;

        if (a_ptr->gen_flags & TRG_QUESTITEM)
            continue;

        if (a_ptr->gen_flags & TRG_INSTA_ART)
            continue;

        if (a_ptr->tval != o_ptr->tval)
            continue;

        if (a_ptr->sval != o_ptr->sval)
            continue;

        if (a_ptr->level > floor_ptr->dun_level) {
            int d = (a_ptr->level - floor_ptr->dun_level) * 2;
            if (!one_in_(d))
                continue;
        }

        if (!one_in_(a_ptr->rarity))
            continue;

        o_ptr->name1 = i;
        random_artifact_resistance(player_ptr, o_ptr, a_ptr);
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する。
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 生成に割り当てたいオブジェクトの構造体参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * We are only called from "make_object()", and we assume that\n
 * "apply_magic()" is called immediately after we return.\n
 *\n
 * Note -- see "make_artifact()" and "apply_magic()"\n
 */
bool make_artifact_special(player_type *player_ptr, object_type *o_ptr)
{
    KIND_OBJECT_IDX k_idx = 0;

    /*! @note 地上ではキャンセルする / No artifacts in the town */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level == 0)
        return FALSE;

    /*! @note get_obj_num_hookによる指定がある場合は生成をキャンセルする / Themed object */
    if (get_obj_num_hook)
        return FALSE;

    /*! @note 全固定アーティファクト中からIDの若い順に生成対象とその確率を走査する / Check the artifact list (just the "specials") */
    for (ARTIFACT_IDX i = 0; i < max_a_idx; i++) {
        artifact_type *a_ptr = &a_info[i];

        /*! @note アーティファクト名が空の不正なデータは除外する / Skip "empty" artifacts */
        if (!a_ptr->name)
            continue;

        /*! @note 既に生成回数がカウントされたアーティファクト、QUESTITEMと非INSTA_ARTは除外 / Cannot make an artifact twice */
        if (a_ptr->cur_num)
            continue;
        if (a_ptr->gen_flags & TRG_QUESTITEM)
            continue;
        if (!(a_ptr->gen_flags & TRG_INSTA_ART))
            continue;

        /*! @note アーティファクト生成階が現在に対して足りない場合は高確率で1/(不足階層*2)を満たさないと生成リストに加えられない /
         *  XXX XXX Enforce minimum "depth" (loosely) */
        if (a_ptr->level > floor_ptr->object_level) {
            /* @note  / Acquire the "out-of-depth factor". Roll for out-of-depth creation. */
            int d = (a_ptr->level - floor_ptr->object_level) * 2;
            if (!one_in_(d))
                continue;
        }

        /*! @note 1/(レア度)の確率を満たさないと除外される / Artifact "rarity roll" */
        if (!one_in_(a_ptr->rarity))
            continue;

        /*! @note INSTA_ART型固定アーティファクトのベースアイテムもチェック対象とする。ベースアイテムの生成階層が足りない場合1/(不足階層*5)
         * を満たさないと除外される。 / Find the base object. XXX XXX Enforce minimum "object" level (loosely). Acquire the "out-of-depth factor". Roll for
         * out-of-depth creation. */
        k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
        if (k_info[k_idx].level > floor_ptr->object_level) {
            int d = (k_info[k_idx].level - floor_ptr->object_level) * 5;
            if (!one_in_(d))
                continue;
        }

        /*! @note 前述の条件を満たしたら、後のIDのアーティファクトはチェックせずすぐ確定し生成処理に移す /
         * Assign the template. Mega-Hack -- mark the item as an artifact. Hack: Some artifacts get random extra powers. Success. */
        object_prep(player_ptr, o_ptr, k_idx);

        o_ptr->name1 = i;
        random_artifact_resistance(player_ptr, o_ptr, a_ptr);
        return TRUE;
    }

    /*! @note 全INSTA_ART固定アーティファクトを試行しても決まらなかった場合 FALSEを返す / Failure */
    return FALSE;
}
