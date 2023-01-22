#include "wizard/artifact-analyzer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "locale/japanese.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "system/artifact-type-definition.h"
#include "system/item-entity.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "wizard/spoiler-util.h"

/*!
 * @brief アーティファクトの特性一覧を出力する /
 * Write a line to the spoiler file and then "underline" it with hypens
 * @param art_flags アーティファクトのフラグ群
 * @param flag_ptr フラグ記述情報の参照ポインタ
 * @param desc_ptr 記述内容を返すための文字列参照ポインタ
 * @param n_elmnts フラグの要素数
 * @return desc_ptrと同じアドレス
 * @details
 * <pre>
 * This function does most of the actual "analysis". Given a set of bit flags
 * (which will be from one of the flags fields from the object in question),
 * a "flag description structure", a "description list", and the number of
 * elements in the "flag description structure", this function sets the
 * "description list" members to the appropriate descriptions contained in
 * the "flag description structure".
 * The possibly updated description pointer is returned.
 * </pre>
 */
static concptr *spoiler_flag_aux(const TrFlags &art_flags, const flag_desc *flag_ptr, concptr *desc_ptr, const int n_elmnts)
{
    for (int i = 0; i < n_elmnts; ++i) {
        if (art_flags.has(flag_ptr[i].flag)) {
            *desc_ptr++ = flag_ptr[i].desc;
        }
    }

    return desc_ptr;
}

/*!
 * @brief アイテムの特定記述内容を返す /
 * Acquire a "basic" description "The Cloak of Death [1,+10]"
 * @param o_ptr 記述を得たいオブジェクトの参照ポインタ
 * @param desc_ptr 記述内容を返すための文字列参照ポインタ
 */
static void analyze_general(PlayerType *player_ptr, ItemEntity *o_ptr, char *desc_ptr)
{
    describe_flavor(player_ptr, desc_ptr, o_ptr, OD_NAME_AND_ENCHANT | OD_STORE | OD_DEBUG);
}

/*!
 * @brief アーティファクトがプレイヤーに与えるpval修正を構造体に収める /
 * List "player traits" altered by an artifact's pval. These include stats,
 * speed, infravision, tunneling, stealth, searching, and extra attacks.
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param pi_ptr pval修正構造体の参照ポインタ
 */
static void analyze_pval(ItemEntity *o_ptr, pval_info_type *pi_ptr)
{
    concptr *affects_list;
    if (!o_ptr->pval) {
        pi_ptr->pval_desc[0] = '\0';
        return;
    }

    auto flags = object_flags(o_ptr);
    affects_list = pi_ptr->pval_affects;
    strnfmt(pi_ptr->pval_desc, sizeof(pi_ptr->pval_desc), "%s%d", o_ptr->pval >= 0 ? "+" : "", o_ptr->pval);
    if (flags.has_all_of(EnumRange(TR_STR, TR_CHR))) {
        *affects_list++ = _("全能力", "All stats");
    } else if (flags.has_any_of(EnumRange(TR_STR, TR_CHR))) {
        affects_list = spoiler_flag_aux(flags, stat_flags_desc, affects_list, N_ELEMENTS(stat_flags_desc));
    }

    affects_list = spoiler_flag_aux(flags, pval_flags1_desc, affects_list, N_ELEMENTS(pval_flags1_desc));
    *affects_list = nullptr;
}

/*!
 * @brief アーティファクトの種族スレイ特性を構造体に収める /
 * Note the slaying specialties of a weapon
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param slay_list 種族スレイ構造体の参照ポインタ
 */
static void analyze_slay(ItemEntity *o_ptr, concptr *slay_list)
{
    auto flags = object_flags(o_ptr);
    slay_list = spoiler_flag_aux(flags, slay_flags_desc, slay_list, N_ELEMENTS(slay_flags_desc));
    *slay_list = nullptr;
}

/*!
 * @brief アーティファクトの属性ブランド特性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param brand_list 属性ブランド構造体の参照ポインタ
 */
static void analyze_brand(ItemEntity *o_ptr, concptr *brand_list)
{
    auto flags = object_flags(o_ptr);
    brand_list = spoiler_flag_aux(flags, brand_flags_desc, brand_list, N_ELEMENTS(brand_flags_desc));
    *brand_list = nullptr;
}

/*!
 * @brief アーティファクトの通常耐性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param resist_list 通常耐性構造体の参照ポインタ
 */
static void analyze_resist(ItemEntity *o_ptr, concptr *resist_list)
{
    auto flags = object_flags(o_ptr);
    resist_list = spoiler_flag_aux(flags, resist_flags_desc, resist_list, N_ELEMENTS(resist_flags_desc));
    *resist_list = nullptr;
}

/*!
 * @brief アーティファクトの免疫特性を構造体に収める /
 * Note the immunities granted by an object
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param immune_list 免疫構造体の参照ポインタ
 */
static void analyze_immune(ItemEntity *o_ptr, concptr *immune_list)
{
    auto flags = object_flags(o_ptr);
    immune_list = spoiler_flag_aux(flags, immune_flags_desc, immune_list, N_ELEMENTS(immune_flags_desc));
    *immune_list = nullptr;
}

/*!
 * @brief アーティファクトの弱点付与を構造体に収める /
 * Note the immunities granted by an object
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param immune_list 弱点構造体の参照ポインタ
 */
static void analyze_vulnerable(ItemEntity *o_ptr, concptr *vulnerable_list)
{
    auto flags = object_flags(o_ptr);
    vulnerable_list = spoiler_flag_aux(flags, vulnerable_flags_desc, vulnerable_list, N_ELEMENTS(vulnerable_flags_desc));
    *vulnerable_list = nullptr;
}

/*!
 * @brief アーティファクトの維持特性を構造体に収める /
 * Note which stats an object sustains
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param sustain_list 維持特性構造体の参照ポインタ
 */
static void analyze_sustains(ItemEntity *o_ptr, concptr *sustain_list)
{
    auto flags = object_flags(o_ptr);
    if (flags.has_all_of(EnumRange(TR_SUST_STR, TR_SUST_CHR))) {
        *sustain_list++ = _("全能力", "All stats");
    } else if (flags.has_any_of(EnumRange(TR_SUST_STR, TR_SUST_CHR))) {
        sustain_list = spoiler_flag_aux(flags, sustain_flags_desc, sustain_list, N_ELEMENTS(sustain_flags_desc));
    }

    *sustain_list = nullptr;
}

/*!
 * @brief アーティファクトのその他の特性を構造体に収める /
 * Note miscellaneous powers bestowed by an artifact such as see invisible,
 * free action, permanent light, etc.
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_list その他の特性構造体の参照ポインタ
 */
static void analyze_misc_magic(ItemEntity *o_ptr, concptr *misc_list)
{
    auto flags = object_flags(o_ptr);
    misc_list = spoiler_flag_aux(flags, misc_flags2_desc, misc_list, N_ELEMENTS(misc_flags2_desc));
    misc_list = spoiler_flag_aux(flags, misc_flags3_desc, misc_list, N_ELEMENTS(misc_flags3_desc));
    POSITION rad = 0;
    if (flags.has(TR_LITE_1)) {
        rad += 1;
    }

    if (flags.has(TR_LITE_2)) {
        rad += 2;
    }

    if (flags.has(TR_LITE_3)) {
        rad += 3;
    }

    if (flags.has(TR_LITE_M1)) {
        rad -= 1;
    }

    if (flags.has(TR_LITE_M2)) {
        rad -= 2;
    }

    if (flags.has(TR_LITE_M3)) {
        rad -= 3;
    }

    if (o_ptr->ego_idx == EgoType::LITE_SHINE) {
        rad++;
    }

    std::string desc;
    if (flags.has(TR_LITE_FUEL)) {
        if (rad > 0) {
            desc = format(_("それは燃料補給によって明かり(半径 %d)を授ける。", "It provides light (radius %d) when fueled."), (int)rad);
        }
    } else {
        if (rad > 0) {
            desc = format(_("永久光源(半径 %d)", "Permanent Light(radius %d)"), (int)rad);
        } else if (rad < 0) {
            desc = format(_("永久光源(半径-%d)。", "Permanent Light(radius -%d)"), (int)-rad);
        }
    }

    if (rad != 0) {
        *misc_list++ = quark_str(quark_add(desc.data()));
    }

    if (flags.has(TR_TY_CURSE)) {
        *misc_list++ = _("太古の怨念", "Ancient Curse");
    }

    if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
        *misc_list++ = _("永遠の呪い", "Permanently Cursed");
    } else if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        *misc_list++ = _("強力な呪い", "Heavily Cursed");
    } else if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
        *misc_list++ = _("呪い", "Cursed");
    }

    if (flags.has(TR_ADD_L_CURSE)) {
        *misc_list++ = _("呪いを増やす", "Cursing");
    }

    if (flags.has(TR_ADD_H_CURSE)) {
        *misc_list++ = _("強力な呪いを増やす", "Heavily Cursing");
    }

    *misc_list = nullptr;
}

/*!
 * @brief アーティファクトの追加ランダム特性を構造体に収める /
 * Note additional ability and/or resistance of fixed artifacts
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param addition 追加ランダム耐性構造体の参照ポインタ
 * @param addition_sz addition に書き込めるバイト数
 */
static void analyze_addition(ItemEntity *o_ptr, char *addition, size_t addition_sz)
{
    const auto &a_ref = artifacts_info.at(o_ptr->fixed_artifact_idx);
    strcpy(addition, "");

    if (a_ref.gen_flags.has_all_of({ ItemGenerationTraitType::XTRA_POWER, ItemGenerationTraitType::XTRA_H_RES })) {
        angband_strcat(addition, _("能力and耐性", "Ability and Resistance"), addition_sz);
    } else if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_POWER)) {
        angband_strcat(addition, _("能力", "Ability"), addition_sz);
        if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_RES_OR_POWER)) {
            angband_strcat(addition, _("(1/2でand耐性)", "(plus Resistance about 1/2)"), addition_sz);
        }
    } else if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_H_RES)) {
        angband_strcat(addition, _("耐性", "Resistance"), addition_sz);
        if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_RES_OR_POWER)) {
            angband_strcat(addition, _("(1/2でand能力)", "(plus Ability about 1/2)"), addition_sz);
        }
    } else if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_RES_OR_POWER)) {
        angband_strcat(addition, _("能力or耐性", "Ability or Resistance"), addition_sz);
    }

    if (a_ref.gen_flags.has(ItemGenerationTraitType::XTRA_DICE)) {
        if (strlen(addition) > 0) {
            angband_strcat(addition, _("、", ", "), addition_sz);
        }
        angband_strcat(addition, _("ダイス数", "Dice number"), addition_sz);
    }
}

/*!
 * @brief アーティファクトの基本情報を文字列に収める /
 * Determine the minimum depth an artifact can appear, its rarity, its weight,
 * and its value in gold pieces
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_desc 基本情報を収める文字列参照ポインタ
 * @param misc_desc_sz misc_desc に書き込めるバイト数
 */
static void analyze_misc(ItemEntity *o_ptr, char *misc_desc, size_t misc_desc_sz)
{
    const auto &a_ref = artifacts_info.at(o_ptr->fixed_artifact_idx);
    strnfmt(misc_desc, misc_desc_sz, _("レベル %d, 希少度 %u, %d.%d kg, ＄%ld", "Level %d, Rarity %u, %d.%d lbs, %ld Gold"), (int)a_ref.level, a_ref.rarity,
        _(lb_to_kg_integer(a_ref.weight), a_ref.weight / 10), _(lb_to_kg_fraction(a_ref.weight), a_ref.weight % 10), (long int)a_ref.cost);
}

/*!
 * @brief アーティファクトの情報全体を構造体に収める /
 * Fill in an object description structure for a given object
 * and its value in gold pieces
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param desc_ptr 全アーティファクト情報を収める文字列参照ポインタ
 */
void object_analyze(PlayerType *player_ptr, ItemEntity *o_ptr, obj_desc_list *desc_ptr)
{
    analyze_general(player_ptr, o_ptr, desc_ptr->description);
    analyze_pval(o_ptr, &desc_ptr->pval_info);
    analyze_brand(o_ptr, desc_ptr->brands);
    analyze_slay(o_ptr, desc_ptr->slays);
    analyze_immune(o_ptr, desc_ptr->immunities);
    analyze_resist(o_ptr, desc_ptr->resistances);
    analyze_vulnerable(o_ptr, desc_ptr->vulnerables);
    analyze_sustains(o_ptr, desc_ptr->sustains);
    analyze_misc_magic(o_ptr, desc_ptr->misc_magic);
    analyze_addition(o_ptr, desc_ptr->addition, sizeof(desc_ptr->addition));
    analyze_misc(o_ptr, desc_ptr->misc_desc, sizeof(desc_ptr->misc_desc));
    desc_ptr->activation = activation_explanation(o_ptr);
}

/*!
 * @brief ランダムアーティファクト１件を解析する /
 * Fill in an object description structure for a given object
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param desc_ptr 記述内容を収める構造体参照ポインタ
 */
void random_artifact_analyze(PlayerType *player_ptr, ItemEntity *o_ptr, obj_desc_list *desc_ptr)
{
    analyze_general(player_ptr, o_ptr, desc_ptr->description);
    analyze_pval(o_ptr, &desc_ptr->pval_info);
    analyze_brand(o_ptr, desc_ptr->brands);
    analyze_slay(o_ptr, desc_ptr->slays);
    analyze_immune(o_ptr, desc_ptr->immunities);
    analyze_resist(o_ptr, desc_ptr->resistances);
    analyze_vulnerable(o_ptr, desc_ptr->vulnerables);
    analyze_sustains(o_ptr, desc_ptr->sustains);
    analyze_misc_magic(o_ptr, desc_ptr->misc_magic);
    desc_ptr->activation = activation_explanation(o_ptr);
    strnfmt(desc_ptr->misc_desc, sizeof(desc_ptr->misc_desc), _("重さ %d.%d kg", "Weight %d.%d lbs"), _(lb_to_kg_integer(o_ptr->weight), o_ptr->weight / 10),
        _(lb_to_kg_fraction(o_ptr->weight), o_ptr->weight % 10));
}
