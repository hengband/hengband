#include "wizard/artifact-analyzer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
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
    for (int i = 0; i < n_elmnts; ++i)
        if (has_flag(art_flags, flag_ptr[i].flag))
            *desc_ptr++ = flag_ptr[i].desc;

    return desc_ptr;
}

/*!
 * @brief アイテムの特定記述内容を返す /
 * Acquire a "basic" description "The Cloak of Death [1,+10]"
 * @param o_ptr 記述を得たいオブジェクトの参照ポインタ
 * @param desc_ptr 記述内容を返すための文字列参照ポインタ
 */
static void analyze_general(player_type *player_ptr, object_type *o_ptr, char *desc_ptr)
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
static void analyze_pval(object_type *o_ptr, pval_info_type *pi_ptr)
{
    TrFlags flgs;
    concptr *affects_list;
    if (!o_ptr->pval) {
        pi_ptr->pval_desc[0] = '\0';
        return;
    }

    object_flags(o_ptr, flgs);
    affects_list = pi_ptr->pval_affects;
    sprintf(pi_ptr->pval_desc, "%s%d", o_ptr->pval >= 0 ? "+" : "", o_ptr->pval);
    if (has_flag(flgs, TR_STR) && has_flag(flgs, TR_INT) && has_flag(flgs, TR_WIS) && has_flag(flgs, TR_DEX) && has_flag(flgs, TR_CON)
        && has_flag(flgs, TR_CHR)) {
        *affects_list++ = _("全能力", "All stats");
    } else if (has_flag(flgs, TR_STR) || has_flag(flgs, TR_INT) || has_flag(flgs, TR_WIS) || has_flag(flgs, TR_DEX) || has_flag(flgs, TR_CON)
        || has_flag(flgs, TR_CHR)) {
        affects_list = spoiler_flag_aux(flgs, stat_flags_desc, affects_list, N_ELEMENTS(stat_flags_desc));
    }

    affects_list = spoiler_flag_aux(flgs, pval_flags1_desc, affects_list, N_ELEMENTS(pval_flags1_desc));
    *affects_list = NULL;
}

/*!
 * @brief アーティファクトの種族スレイ特性を構造体に収める /
 * Note the slaying specialties of a weapon
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param slay_list 種族スレイ構造体の参照ポインタ
 */
static void analyze_slay(object_type *o_ptr, concptr *slay_list)
{
    TrFlags flgs;
    object_flags(o_ptr, flgs);
    slay_list = spoiler_flag_aux(flgs, slay_flags_desc, slay_list, N_ELEMENTS(slay_flags_desc));
    *slay_list = NULL;
}

/*!
 * @brief アーティファクトの属性ブランド特性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param brand_list 属性ブランド構造体の参照ポインタ
 */
static void analyze_brand(object_type *o_ptr, concptr *brand_list)
{
    TrFlags flgs;
    object_flags(o_ptr, flgs);
    brand_list = spoiler_flag_aux(flgs, brand_flags_desc, brand_list, N_ELEMENTS(brand_flags_desc));
    *brand_list = NULL;
}

/*!
 * @brief アーティファクトの通常耐性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param resist_list 通常耐性構造体の参照ポインタ
 */
static void analyze_resist(object_type *o_ptr, concptr *resist_list)
{
    TrFlags flgs;
    object_flags(o_ptr, flgs);
    resist_list = spoiler_flag_aux(flgs, resist_flags_desc, resist_list, N_ELEMENTS(resist_flags_desc));
    *resist_list = NULL;
}

/*!
 * @brief アーティファクトの免疫特性を構造体に収める /
 * Note the immunities granted by an object
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param immune_list 免疫構造体の参照ポインタ
 */
static void analyze_immune(object_type *o_ptr, concptr *immune_list)
{
    TrFlags flgs;
    object_flags(o_ptr, flgs);
    immune_list = spoiler_flag_aux(flgs, immune_flags_desc, immune_list, N_ELEMENTS(immune_flags_desc));
    *immune_list = NULL;
}

/*!
 * @brief アーティファクトの維持特性を構造体に収める /
 * Note which stats an object sustains
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param sustain_list 維持特性構造体の参照ポインタ
 */
static void analyze_sustains(object_type *o_ptr, concptr *sustain_list)
{
    TrFlags flgs;
    object_flags(o_ptr, flgs);
    if (has_flag(flgs, TR_SUST_STR) && has_flag(flgs, TR_SUST_INT) && has_flag(flgs, TR_SUST_WIS) && has_flag(flgs, TR_SUST_DEX)
        && has_flag(flgs, TR_SUST_CON) && has_flag(flgs, TR_SUST_CHR)) {
        *sustain_list++ = _("全能力", "All stats");
    } else if (has_flag(flgs, TR_SUST_STR) || has_flag(flgs, TR_SUST_INT) || has_flag(flgs, TR_SUST_WIS) || has_flag(flgs, TR_SUST_DEX)
        || has_flag(flgs, TR_SUST_CON) || has_flag(flgs, TR_SUST_CHR)) {
        sustain_list = spoiler_flag_aux(flgs, sustain_flags_desc, sustain_list, N_ELEMENTS(sustain_flags_desc));
    }

    *sustain_list = NULL;
}

/*!
 * @brief アーティファクトのその他の特性を構造体に収める /
 * Note miscellaneous powers bestowed by an artifact such as see invisible,
 * free action, permanent light, etc.
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_list その他の特性構造体の参照ポインタ
 */
static void analyze_misc_magic(object_type *o_ptr, concptr *misc_list)
{
    TrFlags flgs;
    char desc[256];

    object_flags(o_ptr, flgs);
    misc_list = spoiler_flag_aux(flgs, misc_flags2_desc, misc_list, N_ELEMENTS(misc_flags2_desc));
    misc_list = spoiler_flag_aux(flgs, misc_flags3_desc, misc_list, N_ELEMENTS(misc_flags3_desc));
    POSITION rad = 0;
    if (has_flag(flgs, TR_LITE_1))
        rad += 1;

    if (has_flag(flgs, TR_LITE_2))
        rad += 2;

    if (has_flag(flgs, TR_LITE_3))
        rad += 3;

    if (has_flag(flgs, TR_LITE_M1))
        rad -= 1;

    if (has_flag(flgs, TR_LITE_M2))
        rad -= 2;

    if (has_flag(flgs, TR_LITE_M3))
        rad -= 3;

    if (o_ptr->name2 == EGO_LITE_SHINE)
        rad++;

    if (has_flag(flgs, TR_LITE_FUEL)) {
        if (rad > 0)
            sprintf(desc, _("それは燃料補給によって明かり(半径 %d)を授ける。", "It provides light (radius %d) when fueled."), (int)rad);
    } else {
        if (rad > 0)
            sprintf(desc, _("永久光源(半径 %d)", "Permanent Light(radius %d)"), (int)rad);

        if (rad < 0)
            sprintf(desc, _("永久光源(半径-%d)。", "Permanent Light(radius -%d)"), (int)-rad);
    }

    if (rad != 0)
        *misc_list++ = quark_str(quark_add(desc));

    if (has_flag(flgs, TR_TY_CURSE))
        *misc_list++ = _("太古の怨念", "Ancient Curse");

    if (o_ptr->curse_flags.has(TRC::PERMA_CURSE))
        *misc_list++ = _("永遠の呪い", "Permanently Cursed");
    else if (o_ptr->curse_flags.has(TRC::HEAVY_CURSE))
        *misc_list++ = _("強力な呪い", "Heavily Cursed");
    else if (o_ptr->curse_flags.has(TRC::CURSED))
        *misc_list++ = _("呪い", "Cursed");

    if (has_flag(flgs, TR_ADD_L_CURSE))
        *misc_list++ = _("呪いを増やす", "Cursing");

    if (has_flag(flgs, TR_ADD_H_CURSE))
        *misc_list++ = _("強力な呪いを増やす", "Heavily Cursing");

    *misc_list = NULL;
}

/*!
 * @brief アーティファクトの追加ランダム特性を構造体に収める /
 * Note additional ability and/or resistance of fixed artifacts
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param addition 追加ランダム耐性構造体の参照ポインタ
 */
static void analyze_addition(object_type *o_ptr, char *addition)
{
    artifact_type *a_ptr = &a_info[o_ptr->name1];
    strcpy(addition, "");

    if (a_ptr->gen_flags.has_all_of({ TRG::XTRA_POWER, TRG::XTRA_H_RES })) {
        strcat(addition, _("能力and耐性", "Ability and Resistance"));
    } else if (a_ptr->gen_flags.has(TRG::XTRA_POWER)) {
        strcat(addition, _("能力", "Ability"));
        if (a_ptr->gen_flags.has(TRG::XTRA_RES_OR_POWER))
            strcat(addition, _("(1/2でand耐性)", "(plus Resistance about 1/2)"));
    } else if (a_ptr->gen_flags.has(TRG::XTRA_H_RES)) {
        strcat(addition, _("耐性", "Resistance"));
        if (a_ptr->gen_flags.has(TRG::XTRA_RES_OR_POWER))
            strcat(addition, _("(1/2でand能力)", "(plus Ability about 1/2)"));
    } else if (a_ptr->gen_flags.has(TRG::XTRA_RES_OR_POWER))
        strcat(addition, _("能力or耐性", "Ability or Resistance"));

    if (a_ptr->gen_flags.has(TRG::XTRA_DICE)) {
        if (strlen(addition) > 0)
            strcat(addition, _("、", ", "));
        strcat(addition, _("ダイス数", "Dice number"));
    }
}

/*!
 * @brief アーティファクトの基本情報を文字列に収める /
 * Determine the minimum depth an artifact can appear, its rarity, its weight,
 * and its value in gold pieces
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_desc 基本情報を収める文字列参照ポインタ
 */
static void analyze_misc(object_type *o_ptr, char *misc_desc)
{
    artifact_type *a_ptr = &a_info[o_ptr->name1];
    sprintf(misc_desc, _("レベル %d, 希少度 %u, %d.%d kg, ＄%ld", "Level %d, Rarity %u, %d.%d lbs, %ld Gold"), (int)a_ptr->level, a_ptr->rarity,
        _(lbtokg1(a_ptr->weight), a_ptr->weight / 10), _(lbtokg2(a_ptr->weight), a_ptr->weight % 10), (long int)a_ptr->cost);
}

/*!
 * @brief アーティファクトの情報全体を構造体に収める /
 * Fill in an object description structure for a given object
 * and its value in gold pieces
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param desc_ptr 全アーティファクト情報を収める文字列参照ポインタ
 */
void object_analyze(player_type *player_ptr, object_type *o_ptr, obj_desc_list *desc_ptr)
{
    analyze_general(player_ptr, o_ptr, desc_ptr->description);
    analyze_pval(o_ptr, &desc_ptr->pval_info);
    analyze_brand(o_ptr, desc_ptr->brands);
    analyze_slay(o_ptr, desc_ptr->slays);
    analyze_immune(o_ptr, desc_ptr->immunities);
    analyze_resist(o_ptr, desc_ptr->resistances);
    analyze_sustains(o_ptr, desc_ptr->sustains);
    analyze_misc_magic(o_ptr, desc_ptr->misc_magic);
    analyze_addition(o_ptr, desc_ptr->addition);
    analyze_misc(o_ptr, desc_ptr->misc_desc);
    desc_ptr->activation = activation_explanation(o_ptr);
}

/*!
 * @brief ランダムアーティファクト１件を解析する /
 * Fill in an object description structure for a given object
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param desc_ptr 記述内容を収める構造体参照ポインタ
 */
void random_artifact_analyze(player_type *player_ptr, object_type *o_ptr, obj_desc_list *desc_ptr)
{
    analyze_general(player_ptr, o_ptr, desc_ptr->description);
    analyze_pval(o_ptr, &desc_ptr->pval_info);
    analyze_brand(o_ptr, desc_ptr->brands);
    analyze_slay(o_ptr, desc_ptr->slays);
    analyze_immune(o_ptr, desc_ptr->immunities);
    analyze_resist(o_ptr, desc_ptr->resistances);
    analyze_sustains(o_ptr, desc_ptr->sustains);
    analyze_misc_magic(o_ptr, desc_ptr->misc_magic);
    desc_ptr->activation = activation_explanation(o_ptr);
    sprintf(desc_ptr->misc_desc, _("重さ %d.%d kg", "Weight %d.%d lbs"), _(lbtokg1(o_ptr->weight), o_ptr->weight / 10),
        _(lbtokg2(o_ptr->weight), o_ptr->weight % 10));
}
