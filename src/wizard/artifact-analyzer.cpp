#include "wizard/artifact-analyzer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "locale/japanese.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-info.h"
#include "system/artifact-type-definition.h"
#include "system/item-entity.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include "util/string-processor.h"
#include "wizard/spoiler-util.h"
#include <sstream>

/*!
 * @brief アイテムの特定記述内容を返す /
 * Acquire a "basic" description "The Cloak of Death [1,+10]"
 * @param o_ptr 記述を得たいオブジェクトの参照ポインタ
 * @param desc_ptr 記述内容を返すための文字列参照ポインタ
 */
static std::string analyze_general(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    return describe_flavor(player_ptr, o_ptr, OD_NAME_AND_ENCHANT | OD_STORE | OD_DEBUG);
}

/*!
 * @brief アーティファクトの種族スレイ特性を構造体に収める /
 * Note the slaying specialties of a weapon
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param slay_list 種族スレイ構造体の参照ポインタ
 */
static std::vector<std::string> analyze_slay(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    return extract_spoiler_flags(flags, slay_flags_desc);
}

/*!
 * @brief アーティファクトの属性ブランド特性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param brand_list 属性ブランド構造体の参照ポインタ
 */
static std::vector<std::string> analyze_brand(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    return extract_spoiler_flags(flags, brand_flags_desc);
}

/*!
 * @brief アーティファクトの通常耐性を構造体に収める /
 * Note an object's elemental brands
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param resist_list 通常耐性構造体の参照ポインタ
 */
static std::vector<std::string> analyze_resist(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    return extract_spoiler_flags(flags, resist_flags_desc);
}

/*!
 * @brief アーティファクトの免疫特性を構造体に収める /
 * Note the immunities granted by an object
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param immune_list 免疫構造体の参照ポインタ
 */
static std::vector<std::string> analyze_immune(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    return extract_spoiler_flags(flags, immune_flags_desc);
}

/*!
 * @brief アーティファクトの弱点付与を構造体に収める /
 * Note the immunities granted by an object
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param immune_list 弱点構造体の参照ポインタ
 */
static std::vector<std::string> analyze_vulnerable(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    return extract_spoiler_flags(flags, vulnerable_flags_desc);
}

/*!
 * @brief アーティファクトの維持特性を構造体に収める /
 * Note which stats an object sustains
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param sustain_list 維持特性構造体の参照ポインタ
 */
static std::vector<std::string> analyze_sustains(const ItemEntity *o_ptr)
{
    const auto flags = o_ptr->get_flags();
    if (flags.has_all_of(EnumRange(TR_SUST_STR, TR_SUST_CHR))) {
        return { _("全能力", "All stats") };
    }

    if (flags.has_any_of(EnumRange(TR_SUST_STR, TR_SUST_CHR))) {
        return extract_spoiler_flags(flags, sustain_flags_desc);
    }

    return {};
}

/*!
 * @brief アーティファクトのその他の特性を構造体に収める /
 * Note miscellaneous powers bestowed by an artifact such as see invisible,
 * free action, permanent light, etc.
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_list その他の特性構造体の参照ポインタ
 */
static std::vector<std::string> analyze_misc_magic(const ItemEntity *o_ptr)
{
    std::vector<std::string> descriptions{};
    const auto flags = o_ptr->get_flags();
    const auto &flags2_descriptions = extract_spoiler_flags(flags, misc_flags2_desc);
    descriptions.insert(descriptions.end(), flags2_descriptions.begin(), flags2_descriptions.end());
    const auto &flags3_descriptions = extract_spoiler_flags(flags, misc_flags3_desc);
    descriptions.insert(descriptions.end(), flags3_descriptions.begin(), flags3_descriptions.end());
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
        descriptions.push_back(std::move(desc));
    }

    if (flags.has(TR_TY_CURSE)) {
        descriptions.emplace_back(_("太古の怨念", "Ancient Curse"));
    }

    if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
        descriptions.emplace_back(_("永遠の呪い", "Permanently Cursed"));
    } else if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        descriptions.emplace_back(_("強力な呪い", "Heavily Cursed"));
    } else if (o_ptr->curse_flags.has(CurseTraitType::CURSED)) {
        descriptions.emplace_back(_("呪い", "Cursed"));
    }

    if (flags.has(TR_ADD_L_CURSE)) {
        descriptions.emplace_back(_("呪いを増やす", "Cursing"));
    }

    if (flags.has(TR_ADD_H_CURSE)) {
        descriptions.emplace_back(_("強力な呪いを増やす", "Heavily Cursing"));
    }

    return descriptions;
}

/*!
 * @brief アーティファクトの追加ランダム特性を構造体に収める /
 * Note additional ability and/or resistance of fixed artifacts
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param addition 追加ランダム耐性構造体の参照ポインタ
 * @param addition_sz addition に書き込めるバイト数
 */
static std::string analyze_addition(const ItemEntity *o_ptr)
{
    const auto &artifact = o_ptr->get_fixed_artifact();
    std::stringstream ss;
    if (artifact.gen_flags.has_all_of({ ItemGenerationTraitType::XTRA_POWER, ItemGenerationTraitType::XTRA_H_RES })) {
        ss << _("能力and耐性", "Ability and Resistance");
    } else if (artifact.gen_flags.has(ItemGenerationTraitType::XTRA_POWER)) {
        ss << _("能力", "Ability");
        if (artifact.gen_flags.has(ItemGenerationTraitType::XTRA_RES_OR_POWER)) {
            ss << _("(1/2でand耐性)", "(plus Resistance about 1/2)");
        }
    } else if (artifact.gen_flags.has(ItemGenerationTraitType::XTRA_H_RES)) {
        ss << _("耐性", "Resistance");
        if (artifact.gen_flags.has(ItemGenerationTraitType::XTRA_RES_OR_POWER)) {
            ss << _("(1/2でand能力)", "(plus Ability about 1/2)");
        }
    } else if (artifact.gen_flags.has(ItemGenerationTraitType::XTRA_RES_OR_POWER)) {
        ss << _("能力or耐性", "Ability or Resistance");
    }

    if (artifact.gen_flags.has_not(ItemGenerationTraitType::XTRA_DICE)) {
        return ss.str();
    }

    if (ss.tellp() > 0) {
        ss << _("、", ", ");
    }

    ss << _("ダイス数", "Dice number");
    return ss.str();
}

/*!
 * @brief アーティファクトの基本情報を文字列に収める /
 * Determine the minimum depth an artifact can appear, its rarity, its weight,
 * and its value in gold pieces
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @param misc_desc 基本情報を収める文字列参照ポインタ
 * @param misc_desc_sz misc_desc に書き込めるバイト数
 */
static std::string analyze_misc(const ItemEntity *o_ptr)
{
    const auto &artifact = o_ptr->get_fixed_artifact();
    constexpr auto fmt = _("レベル %d, 希少度 %u, %d.%d kg, ＄%d", "Level %d, Rarity %u, %d.%d lbs, %d Gold");
    const auto weight_integer = _(lb_to_kg_integer(artifact.weight), artifact.weight / 10);
    const auto weight_fraction = _(lb_to_kg_fraction(artifact.weight), artifact.weight % 10);
    return format(fmt, artifact.level, artifact.rarity, weight_integer, weight_fraction, artifact.cost);
}

/*!
 * @brief アーティファクトの情報全体を構造体に収める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr オブジェクト構造体の参照ポインタ
 */
ArtifactsDumpInfo object_analyze(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    ArtifactsDumpInfo info{};
    info.description = analyze_general(player_ptr, o_ptr);
    info.pval_info.analyze(*o_ptr);
    info.brands = analyze_brand(o_ptr);
    info.slays = analyze_slay(o_ptr);
    info.immunities = analyze_immune(o_ptr);
    info.resistances = analyze_resist(o_ptr);
    info.vulnerabilities = analyze_vulnerable(o_ptr);
    info.sustenances = analyze_sustains(o_ptr);
    info.misc_magic = analyze_misc_magic(o_ptr);
    info.addition = analyze_addition(o_ptr);
    info.misc_desc = analyze_misc(o_ptr);
    info.activation = o_ptr->explain_activation();
    return info;
}

/*!
 * @brief ランダムアーティファクト1件を解析する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param desc_ptr 記述内容を収める構造体参照ポインタ
 */
ArtifactsDumpInfo random_artifact_analyze(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    ArtifactsDumpInfo info{};
    info.description = analyze_general(player_ptr, o_ptr);
    info.pval_info.analyze(*o_ptr);
    info.brands = analyze_brand(o_ptr);
    info.slays = analyze_slay(o_ptr);
    info.immunities = analyze_immune(o_ptr);
    info.resistances = analyze_resist(o_ptr);
    info.vulnerabilities = analyze_vulnerable(o_ptr);
    info.sustenances = analyze_sustains(o_ptr);
    info.misc_magic = analyze_misc_magic(o_ptr);
    info.activation = o_ptr->explain_activation();
    constexpr auto weight_mes = _("重さ %d.%d kg", "Weight %d.%d lbs");
    const auto weight_integer = _(lb_to_kg_integer(o_ptr->weight), o_ptr->weight / 10);
    const auto weight_fraction = _(lb_to_kg_fraction(o_ptr->weight), o_ptr->weight % 10);
    info.misc_desc = format(weight_mes, weight_integer, weight_fraction);
    return info;
}
