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
 * @brief アイテムの特定記述内容表記を生成する
 * @param item 記述を得たいアイテムへの参照
 * @return 特定記述内容
 */
static std::string analyze_general(PlayerType *player_ptr, const ItemEntity &item)
{
    return describe_flavor(player_ptr, item, OD_NAME_AND_ENCHANT | OD_STORE | OD_DEBUG);
}

/*!
 * @brief アーティファクトの種族スレイ特性表記を生成する
 * @param item アイテムへの参照
 * @return 種族スレイ
 */
static std::vector<std::string> analyze_slay(const ItemEntity &item)
{
    const auto flags = item.get_flags();
    return extract_spoiler_flags(flags, slay_flags_desc);
}

/*!
 * @brief アーティファクトの属性ブランド特性表記を生成する
 * @param item アイテムへの参照
 * @return 属性ブランド特性
 */
static std::vector<std::string> analyze_brand(const ItemEntity &item)
{
    const auto flags = item.get_flags();
    return extract_spoiler_flags(flags, brand_flags_desc);
}

/*!
 * @brief アーティファクトの通常耐性表記を生成する
 * @param item アイテムへの参照
 * @return 通常耐性
 */
static std::vector<std::string> analyze_resist(const ItemEntity &item)
{
    const auto flags = item.get_flags();
    return extract_spoiler_flags(flags, resist_flags_desc);
}

/*!
 * @brief アーティファクトの免疫特性表記を生成する
 * @param item アイテムへの参照
 * @return 免疫特性
 */
static std::vector<std::string> analyze_immune(const ItemEntity &item)
{
    const auto flags = item.get_flags();
    return extract_spoiler_flags(flags, immune_flags_desc);
}

/*!
 * @brief アーティファクトの弱点付与表記を生成する
 * @param item アイテムへの参照
 * @return 弱点付与
 */
static std::vector<std::string> analyze_vulnerable(const ItemEntity &item)
{
    const auto flags = item.get_flags();
    return extract_spoiler_flags(flags, vulnerable_flags_desc);
}

/*!
 * @brief アーティファクトの維持アビリティスコア表記を生成する
 * @param item アイテムへの参照
 * @return 維持アビリティスコア
 */
static std::vector<std::string> analyze_sustains(const ItemEntity &item)
{
    const auto flags = item.get_flags();
    if (flags.has_all_of(EnumRangeInclusive(TR_SUST_STR, TR_SUST_CHR))) {
        return { _("全能力", "All stats") };
    }

    if (flags.has_any_of(EnumRangeInclusive(TR_SUST_STR, TR_SUST_CHR))) {
        return extract_spoiler_flags(flags, sustain_flags_desc);
    }

    return {};
}

/*!
 * @brief アーティファクトのその他の特性表記を生成する
 * @param item アイテムへの参照
 * @return その他の特性
 */
static std::vector<std::string> analyze_misc_magic(const ItemEntity &item)
{
    std::vector<std::string> descriptions{};
    const auto flags = item.get_flags();
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

    if (item.ego_idx == EgoType::LITE_SHINE) {
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

    if (item.curse_flags.has(CurseTraitType::PERMA_CURSE)) {
        descriptions.emplace_back(_("永遠の呪い", "Permanently Cursed"));
    } else if (item.curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        descriptions.emplace_back(_("強力な呪い", "Heavily Cursed"));
    } else if (item.curse_flags.has(CurseTraitType::CURSED)) {
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
 * @brief アーティファクトの追加ランダム特性表記を生成する
 * @param item アイテムへの参照
 * @return 追加ランダム特性
 */
static std::string analyze_addition(const ItemEntity &item)
{
    const auto &artifact = item.get_fixed_artifact();
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
 * @brief アーティファクトの基本情報表記を生成する
 * @param item アイテムへの参照
 * @return 基本情報
 */
static std::string analyze_misc(const ItemEntity &item)
{
    const auto &artifact = item.get_fixed_artifact();
    constexpr auto fmt = _("レベル %d, 希少度 %u, %d.%d kg, ＄%d", "Level %d, Rarity %u, %d.%d lbs, %d Gold");
    const auto weight_integer = _(lb_to_kg_integer(artifact.weight), artifact.weight / 10);
    const auto weight_fraction = _(lb_to_kg_fraction(artifact.weight), artifact.weight % 10);
    return format(fmt, artifact.level, artifact.rarity, weight_integer, weight_fraction, artifact.cost);
}

/*!
 * @brief 固定アーティファクト情報1件を解析する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item アーティファクトアイテムへの参照
 * @return アーティファクト情報
 */
ArtifactsDumpInfo object_analyze(PlayerType *player_ptr, const ItemEntity &item)
{
    ArtifactsDumpInfo info{};
    info.description = analyze_general(player_ptr, item);
    info.pval_info.analyze(item);
    info.brands = analyze_brand(item);
    info.slays = analyze_slay(item);
    info.immunities = analyze_immune(item);
    info.resistances = analyze_resist(item);
    info.vulnerabilities = analyze_vulnerable(item);
    info.sustenances = analyze_sustains(item);
    info.misc_magic = analyze_misc_magic(item);
    info.addition = analyze_addition(item);
    info.misc_desc = analyze_misc(item);
    info.activation = item.explain_activation();
    return info;
}

/*!
 * @brief ランダムアーティファクト1件を解析する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item アーティファクトアイテムへの参照
 * @return 解析結果
 */
ArtifactsDumpInfo random_artifact_analyze(PlayerType *player_ptr, const ItemEntity &item)
{
    ArtifactsDumpInfo info{};
    info.description = analyze_general(player_ptr, item);
    info.pval_info.analyze(item);
    info.brands = analyze_brand(item);
    info.slays = analyze_slay(item);
    info.immunities = analyze_immune(item);
    info.resistances = analyze_resist(item);
    info.vulnerabilities = analyze_vulnerable(item);
    info.sustenances = analyze_sustains(item);
    info.misc_magic = analyze_misc_magic(item);
    info.activation = item.explain_activation();
    constexpr auto weight_mes = _("重さ %d.%d kg", "Weight %d.%d lbs");
    const auto weight_integer = _(lb_to_kg_integer(item.weight), item.weight / 10);
    const auto weight_fraction = _(lb_to_kg_fraction(item.weight), item.weight % 10);
    info.misc_desc = format(weight_mes, weight_integer, weight_fraction);
    return info;
}
