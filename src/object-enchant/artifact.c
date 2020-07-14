/*!
 * @brief アーティファクトの生成と管理 / Artifact code
 * @date 2013/12/11
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "object-enchant/artifact.h"
#include "artifact/random-art-activation.h"
#include "artifact/random-art-bias-types.h"
#include "artifact/random-art-characteristics.h"
#include "artifact/random-art-pval-investor.h"
#include "art-definition/art-armor-types.h"
#include "art-definition/art-protector-types.h"
#include "art-definition/art-sword-types.h"
#include "art-definition/art-weapon-types.h"
#include "art-definition/random-art-effects.h"
#include "cmd-item/cmd-smith.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "flavor/object-flavor.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-value-calc.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-personalities-types.h"
#include "spell/spells-object.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "wizard/artifact-bias-table.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

/*
 * The artifact arrays
 */
artifact_type *a_info;
char *a_name;
char *a_text;

/*
 * Maximum number of artifacts in a_info.txt
 */
ARTIFACT_IDX max_a_idx;

/*!
 * @brief オブジェクトから能力発動IDを取得する。
 * @details いくつかのケースで定義されている発動効果から、
 * 鍛冶師による付与＞固定アーティファクト＞エゴ＞ランダムアーティファクト＞ベースアイテムの優先順位で走査していく。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果のIDを返す
 */
int activation_index(player_type *player_ptr, object_type *o_ptr)
{
    if (object_is_smith(player_ptr, o_ptr)) {
        switch (o_ptr->xtra3 - 1) {
        case ESSENCE_TMP_RES_ACID:
            return ACT_RESIST_ACID;
        case ESSENCE_TMP_RES_ELEC:
            return ACT_RESIST_ELEC;
        case ESSENCE_TMP_RES_FIRE:
            return ACT_RESIST_FIRE;
        case ESSENCE_TMP_RES_COLD:
            return ACT_RESIST_COLD;
        case TR_IMPACT:
            return ACT_QUAKE;
        }
    }

    if (object_is_fixed_artifact(o_ptr)) {
        if (have_flag(a_info[o_ptr->name1].flags, TR_ACTIVATE)) {
            return a_info[o_ptr->name1].act_idx;
        }
    }

    if (object_is_ego(o_ptr)) {
        if (have_flag(e_info[o_ptr->name2].flags, TR_ACTIVATE)) {
            return e_info[o_ptr->name2].act_idx;
        }
    }

    if (!object_is_random_artifact(o_ptr)) {
        if (have_flag(k_info[o_ptr->k_idx].flags, TR_ACTIVATE)) {
            return k_info[o_ptr->k_idx].act_idx;
        }
    }

    return o_ptr->xtra2;
}

/*!
 * @brief オブジェクトから発動効果構造体のポインタを取得する。
 * @details activation_index() 関数の結果から参照する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果構造体のポインタを返す
 */
const activation_type *find_activation_info(player_type *player_ptr, object_type *o_ptr)
{
    const int index = activation_index(player_ptr, o_ptr);
    const activation_type *p;
    for (p = activation_info; p->flag != NULL; ++p) {
        if (p->index == index) {
            return p;
        }
    }

    return NULL;
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
void random_artifact_resistance(player_type *player_ptr, object_type *o_ptr, artifact_type *a_ptr)
{
    bool give_resistance = FALSE, give_power = FALSE;

    if (o_ptr->name1 == ART_TERROR) {
        bool is_special_class = player_ptr->pclass == CLASS_WARRIOR;
        is_special_class |= player_ptr->pclass == CLASS_ARCHER;
        is_special_class |= player_ptr->pclass == CLASS_CAVALRY;
        is_special_class |= player_ptr->pclass == CLASS_BERSERKER;
        if (is_special_class) {
            give_power = TRUE;
            give_resistance = TRUE;
        } else {
            add_flag(o_ptr->art_flags, TR_AGGRAVATE);
            add_flag(o_ptr->art_flags, TR_TY_CURSE);
            o_ptr->curse_flags |= (TRC_CURSED | TRC_HEAVY_CURSE);
            o_ptr->curse_flags |= get_curse(player_ptr, 2, o_ptr);
            return;
        }
    }

    if (o_ptr->name1 == ART_MURAMASA) {
        if (player_ptr->pclass != CLASS_SAMURAI) {
            add_flag(o_ptr->art_flags, TR_NO_MAGIC);
            o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
        }
    }

    if (o_ptr->name1 == ART_ROBINTON) {
        if (player_ptr->pclass == CLASS_BARD) {
            add_flag(o_ptr->art_flags, TR_DEC_MANA);
        }
    }

    if (o_ptr->name1 == ART_XIAOLONG) {
        if (player_ptr->pclass == CLASS_MONK)
            add_flag(o_ptr->art_flags, TR_BLOWS);
    }

    if (o_ptr->name1 == ART_BLOOD) {
        get_bloody_moon_flags(o_ptr);
    }

    if (o_ptr->name1 == ART_HEAVENLY_MAIDEN) {
        if (player_ptr->psex != SEX_FEMALE) {
            add_flag(o_ptr->art_flags, TR_AGGRAVATE);
        }
    }

    if (o_ptr->name1 == ART_MILIM) {
        if (player_ptr->pseikaku == PERSONALITY_SEXY) {
            o_ptr->pval = 3;
            add_flag(o_ptr->art_flags, TR_STR);
            add_flag(o_ptr->art_flags, TR_INT);
            add_flag(o_ptr->art_flags, TR_WIS);
            add_flag(o_ptr->art_flags, TR_DEX);
            add_flag(o_ptr->art_flags, TR_CON);
            add_flag(o_ptr->art_flags, TR_CHR);
        }
    }

    if (a_ptr->gen_flags & (TRG_XTRA_POWER))
        give_power = TRUE;
    if (a_ptr->gen_flags & (TRG_XTRA_H_RES))
        give_resistance = TRUE;
    if (a_ptr->gen_flags & (TRG_XTRA_RES_OR_POWER)) {
        /* Give a resistance OR a power */
        if (one_in_(2))
            give_resistance = TRUE;
        else
            give_power = TRUE;
    }

    if (give_power) {
        one_ability(o_ptr);
    }

    if (give_resistance) {
        one_high_resistance(o_ptr);
    }
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
    if (a_ptr->gen_flags & TRG_CURSED)
        q_ptr->curse_flags |= (TRC_CURSED);
    if (a_ptr->gen_flags & TRG_HEAVY_CURSE)
        q_ptr->curse_flags |= (TRC_HEAVY_CURSE);
    if (a_ptr->gen_flags & TRG_PERMA_CURSE)
        q_ptr->curse_flags |= (TRC_PERMA_CURSE);
    if (a_ptr->gen_flags & (TRG_RANDOM_CURSE0))
        q_ptr->curse_flags |= get_curse(player_ptr, 0, q_ptr);
    if (a_ptr->gen_flags & (TRG_RANDOM_CURSE1))
        q_ptr->curse_flags |= get_curse(player_ptr, 1, q_ptr);
    if (a_ptr->gen_flags & (TRG_RANDOM_CURSE2))
        q_ptr->curse_flags |= get_curse(player_ptr, 2, q_ptr);

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
