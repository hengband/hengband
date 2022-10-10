/*!
 * @file fixed-art-generator.cpp
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
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind-hook.h"
#include "player-base/player-class.h"
#include "player/player-sex.h"
#include "specific-object/bloody-moon.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info-definition.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 恐怖の仮面への特殊処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体への参照ポインタ
 * @return 追加能力/耐性がもらえるならtrue、もらえないならfalse
 * @details
 * 純戦士系職業は追加能力/耐性がもらえる。
 * それ以外では、反感、太古の怨念、呪いが付き追加能力/耐性はもらえない。
 */
static bool invest_terror_mask(PlayerType *player_ptr, ObjectType *o_ptr)
{
    if (o_ptr->fixed_artifact_idx != FixedArtifactId::TERROR) {
        return false;
    }

    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::ARCHER:
    case PlayerClassType::CAVALRY:
    case PlayerClassType::BERSERKER:
        return true;
    default:
        o_ptr->art_flags.set(TR_AGGRAVATE);
        o_ptr->art_flags.set(TR_TY_CURSE);
        o_ptr->curse_flags.set({ CurseTraitType::CURSED, CurseTraitType::HEAVY_CURSE });
        o_ptr->curse_flags.set(get_curse(2, o_ptr));
        return false;
    }
}

/*!
 * @brief 戦乙女ミリムの危ない水着への特殊処理 (セクシーギャルのみpval追加)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体への参照ポインタ
 */
static void milim_swimsuit(PlayerType *player_ptr, ObjectType *o_ptr)
{
    if ((o_ptr->fixed_artifact_idx != FixedArtifactId::MILIM) || (player_ptr->ppersonality != PERSONALITY_SEXY)) {
        return;
    }

    o_ptr->pval = 3;
    o_ptr->art_flags.set(TR_STR);
    o_ptr->art_flags.set(TR_INT);
    o_ptr->art_flags.set(TR_WIS);
    o_ptr->art_flags.set(TR_DEX);
    o_ptr->art_flags.set(TR_CON);
    o_ptr->art_flags.set(TR_CHR);
}

/*!
 * @brief 特定の固定アーティファクトの条件付き追加能力/耐性を付加する
 * @attention プレイヤーの各種ステータスに依存した処理がある。
 * @todo 折を見て関数名を変更すること。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_ptr 生成する固定アーティファクト構造体ポインタ
 * @details
 * 対象は村正、ロビントンのハープ、龍争虎鬪、ブラッディムーン、羽衣、天女の羽衣、ミリム
 */
static void invest_special_artifact_abilities(PlayerType *player_ptr, ObjectType *o_ptr)
{
    const auto pc = PlayerClass(player_ptr);
    switch (o_ptr->fixed_artifact_idx) {
    case FixedArtifactId::MURAMASA:
        if (!pc.equals(PlayerClassType::SAMURAI)) {
            o_ptr->art_flags.set(TR_NO_MAGIC);
            o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
        }
        return;
    case FixedArtifactId::ROBINTON:
        if (pc.equals(PlayerClassType::BARD)) {
            o_ptr->art_flags.set(TR_DEC_MANA);
        }
        return;
    case FixedArtifactId::XIAOLONG:
        if (pc.equals(PlayerClassType::MONK)) {
            o_ptr->art_flags.set(TR_BLOWS);
        }
        return;
    case FixedArtifactId::BLOOD:
        get_bloody_moon_flags(o_ptr);
        return;
    case FixedArtifactId::HEAVENLY_MAIDEN:
        if (player_ptr->psex != SEX_FEMALE) {
            o_ptr->art_flags.set(TR_AGGRAVATE);
        }
        return;
    case FixedArtifactId::MILIM:
        milim_swimsuit(player_ptr, o_ptr);
        return;
    default:
        break;
    }
}

/*!
 * @brief 固定アーティファクトオブジェクトに追加能力/耐性を付加する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param a_ptr 固定アーティファクト情報への参照ポインタ
 * @param q_ptr オブジェクト情報への参照ポインタ
 */
static void fixed_artifact_random_abilities(PlayerType *player_ptr, const ArtifactType &a_ref, ObjectType *o_ptr)
{
    auto give_power = false;
    auto give_resistance = false;

    if (invest_terror_mask(player_ptr, o_ptr)) {
        give_power = true;
        give_resistance = true;
    }

    invest_special_artifact_abilities(player_ptr, o_ptr);

    if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_POWER)) {
        give_power = true;
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_H_RES)) {
        give_resistance = true;
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_RES_OR_POWER)) {
        if (one_in_(2)) {
            give_resistance = true;
        } else {
            give_power = true;
        }
    }

    if (give_power) {
        one_ability(o_ptr);
    }

    if (give_resistance) {
        one_high_resistance(o_ptr);
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_DICE)) {
        do {
            o_ptr->dd++;
        } while (one_in_(o_ptr->dd));

        if (o_ptr->dd > 9) {
            o_ptr->dd = 9;
        }
    }
}

/*!
 * @brief 固定アーティファクトオブジェクトに呪いフラグを付加する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param a_ptr 固定アーティファクト情報への参照ポインタ
 * @param q_ptr オブジェクト情報への参照ポインタ
 */
static void invest_curse_to_fixed_artifact(const ArtifactType &a_ref, ObjectType *o_ptr)
{
    if (!a_ref.cost) {
        set_bits(o_ptr->ident, IDENT_BROKEN);
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::CURSED)) {
        o_ptr->curse_flags.set(CurseTraitType::CURSED);
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::HEAVY_CURSE)) {
        o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::PERMA_CURSE)) {
        o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE0)) {
        o_ptr->curse_flags.set(get_curse(0, o_ptr));
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE1)) {
        o_ptr->curse_flags.set(get_curse(1, o_ptr));
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE2)) {
        o_ptr->curse_flags.set(get_curse(2, o_ptr));
    }
}

/*!
 * @brief オブジェクトに指定した固定アーティファクトをオブジェクトに割り当てる。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 生成に割り当てたいオブジェクトの構造体参照ポインタ
 * @return 適用したアーティファクト情報への参照ポインタ
 */
void apply_artifact(PlayerType *player_ptr, ObjectType *o_ptr)
{
    auto &a_ref = artifacts_info.at(o_ptr->fixed_artifact_idx);
    o_ptr->pval = a_ref.pval;
    o_ptr->ac = a_ref.ac;
    o_ptr->dd = a_ref.dd;
    o_ptr->ds = a_ref.ds;
    o_ptr->to_a = a_ref.to_a;
    o_ptr->to_h = a_ref.to_h;
    o_ptr->to_d = a_ref.to_d;
    o_ptr->weight = a_ref.weight;
    o_ptr->activation_id = a_ref.act_idx;

    invest_curse_to_fixed_artifact(a_ref, o_ptr);
    fixed_artifact_random_abilities(player_ptr, a_ref, o_ptr);
}

/*!
 * @brief フロアの指定された位置に固定アーティファクトを生成する。 / Create the artifact of the specified number
 * @details 固定アーティファクト構造体から基本ステータスをコピーした後、所定の座標でdrop_item()で落とす。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param a_idx 生成する固定アーティファクト構造体のID
 * @param y アイテムを落とす地点のy座標
 * @param x アイテムを落とす地点のx座標
 * @return 生成が成功したか否か、失敗はIDの不全、ベースアイテムの不全、drop_item()の失敗時に起こる。
 * @attention この処理はdrop_near()内で普通の固定アーティファクトが重ならない性質に依存する.
 * 仮に2個以上存在可能かつ装備品以外の固定アーティファクトが作成されれば
 * drop_near()関数の返り値は信用できなくなる.
 */
bool create_named_art(PlayerType *player_ptr, FixedArtifactId a_idx, POSITION y, POSITION x)
{
    auto &a_ref = artifacts_info.at(a_idx);
    if (a_ref.name.empty()) {
        return false;
    }

    auto i = lookup_kind(a_ref.tval, a_ref.sval);
    if (i == 0) {
        return true;
    }

    ObjectType forge;
    auto q_ptr = &forge;
    q_ptr->prep(i);
    q_ptr->fixed_artifact_idx = a_idx;
    apply_artifact(player_ptr, q_ptr);
    if (drop_near(player_ptr, q_ptr, -1, y, x) == 0) {
        return false;
    }

    a_ref.is_generated = true;
    return true;
}

/*!
 * @brief 非INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する。
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 生成に割り当てたいオブジェクトの構造体参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * Attempt to change an object into an artifact\n
 * This routine should only be called by "apply_magic()"\n
 * Note -- see "make_artifact_special()" and "apply_magic()"\n
 */
bool make_artifact(PlayerType *player_ptr, ObjectType *o_ptr)
{
    auto floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level == 0) {
        return false;
    }

    if (o_ptr->number != 1) {
        return false;
    }

    for (const auto &[a_idx, a_ref] : artifacts_info) {
        if (a_ref.name.empty()) {
            continue;
        }

        if (a_ref.is_generated) {
            continue;
        }

        if (a_ref.gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
            continue;
        }

        if (a_ref.gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
            continue;
        }

        if (a_ref.tval != o_ptr->tval) {
            continue;
        }

        if (a_ref.sval != o_ptr->sval) {
            continue;
        }

        if (a_ref.level > floor_ptr->dun_level) {
            int d = (a_ref.level - floor_ptr->dun_level) * 2;
            if (!one_in_(d)) {
                continue;
            }
        }

        if (!one_in_(a_ref.rarity)) {
            continue;
        }

        o_ptr->fixed_artifact_idx = a_idx;
        return true;
    }

    return false;
}

/*!
 * @brief INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する。
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 生成に割り当てたいオブジェクトの構造体参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * We are only called from "make_object()", and we assume that\n
 * "apply_magic()" is called immediately after we return.\n
 *\n
 * Note -- see "make_artifact()" and "apply_magic()"\n
 */
bool make_artifact_special(PlayerType *player_ptr, ObjectType *o_ptr)
{
    KIND_OBJECT_IDX k_idx = 0;

    /*! @note 地上ではキャンセルする / No artifacts in the town */
    auto floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level == 0) {
        return false;
    }

    /*! @note get_obj_num_hookによる指定がある場合は生成をキャンセルする / Themed object */
    if (get_obj_num_hook) {
        return false;
    }

    /*! @note 全固定アーティファクト中からIDの若い順に生成対象とその確率を走査する / Check the artifact list (just the "specials") */
    for (const auto &[a_idx, a_ref] : artifacts_info) {
        /*! @note アーティファクト名が空の不正なデータは除外する / Skip "empty" artifacts */
        if (a_ref.name.empty()) {
            continue;
        }

        /*! @note 既に生成回数がカウントされたアーティファクト、QUESTITEMと非INSTA_ARTは除外 / Cannot make an artifact twice */
        if (a_ref.is_generated) {
            continue;
        }
        if (a_ref.gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
            continue;
        }
        if (!(a_ref.gen_flags.has(ItemGenerationTraitType::INSTA_ART))) {
            continue;
        }

        /*! @note アーティファクト生成階が現在に対して足りない場合は高確率で1/(不足階層*2)を満たさないと生成リストに加えられない */
        if (a_ref.level > floor_ptr->object_level) {
            /* @note  / Acquire the "out-of-depth factor". Roll for out-of-depth creation. */
            int d = (a_ref.level - floor_ptr->object_level) * 2;
            if (!one_in_(d)) {
                continue;
            }
        }

        /*! @note 1/(レア度)の確率を満たさないと除外される / Artifact "rarity roll" */
        if (!one_in_(a_ref.rarity)) {
            continue;
        }

        /*!
         * @note INSTA_ART型固定アーティファクトのベースアイテムもチェック対象とする。
         * ベースアイテムの生成階層が足りない場合1/(不足階層*5)を満たさないと除外される。
         */
        k_idx = lookup_kind(a_ref.tval, a_ref.sval);
        if (baseitems_info[k_idx].level > floor_ptr->object_level) {
            int d = (baseitems_info[k_idx].level - floor_ptr->object_level) * 5;
            if (!one_in_(d)) {
                continue;
            }
        }

        /*! @note 前述の条件を満たしたら、後のIDのアーティファクトはチェックせずすぐ確定し生成処理に移す /
         * Assign the template. Mega-Hack -- mark the item as an artifact. Hack: Some artifacts get random extra powers. Success. */
        o_ptr->prep(k_idx);

        o_ptr->fixed_artifact_idx = a_idx;
        return true;
    }

    /*! @note 全INSTA_ART固定アーティファクトを試行しても決まらなかった場合 FALSEを返す / Failure */
    return false;
}
