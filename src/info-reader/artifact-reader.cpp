#include "info-reader/artifact-reader.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/japanese.h"
#include "main/angband-headers.h"
#include "object-enchant/tr-types.h"
#include "system/artifact-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one activation index flag
 * @param a_ptr 保管先のアーティファクト構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つかったらtrue
 */
static bool grab_one_artifact_flag(ArtifactType *a_ptr, std::string_view what)
{
    if (TrFlags::grab_one_flag(a_ptr->flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(a_ptr->gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what.data());
    return false;
}

/*!
 * @brief JSON Objectからアーティファクト名をセットする
 * @param name_data 名前情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_name(const nlohmann::json &name_data, ArtifactType &artifact)
{
    if (name_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

#ifdef JP
    const auto &ja_name = name_data.find("ja");
    if (ja_name == name_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    auto ja_name_sys = utf8_to_sys(ja_name->get<std::string>());
    if (!ja_name_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.name = std::move(*ja_name_sys);
#else
    const auto &en_name = name_data.find("en");
    if (en_name == name_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    artifact.name = en_name->get<std::string>();
#endif

    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトのベースアイテムをセットする
 * @param baseitem_data ベースアイテム情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_baseitem(const nlohmann::json &baseitem_data, ArtifactType &artifact)
{
    if (baseitem_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto &item_type = baseitem_data.find("type_value");
    if (item_type == baseitem_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!item_type->is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto type_value = item_type->get<int>();
    if (type_value < 0 || type_value > 128) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    const auto &item_subtype = baseitem_data.find("subtype_value");
    if (item_subtype == baseitem_data.end()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!item_subtype->is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    const auto subtype_value = item_subtype->get<int>();
    if (subtype_value < 0 || subtype_value > 128) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    artifact.bi_key = { i2enum<ItemKindType>(type_value), subtype_value };
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトのpvalをセットする
 * @param pval_data pval情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_parameter_value(const nlohmann::json &pval_data, ArtifactType &artifact)
{
    if (pval_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!pval_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto parameter_value = pval_data.get<PARAMETER_VALUE>();
    if (parameter_value < -128 || parameter_value > 128) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.pval = parameter_value;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの出現階層をセットする
 * @param level_data レベル情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_level(const nlohmann::json &level_data, ArtifactType &artifact)
{
    if (level_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!level_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto level = level_data.get<int>();
    if (level < 0 || level > 128) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.level = level;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの希少度をセットする
 * @param rarity_data rarity情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_rarity(const nlohmann::json &rarity_data, ArtifactType &artifact)
{
    if (rarity_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!rarity_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto rarity = rarity_data.get<uint8_t>();
    artifact.rarity = rarity;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの重量をセットする
 * @param weight_data weight情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_weight(const nlohmann::json &weight_data, ArtifactType &artifact)
{
    if (weight_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!weight_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto weight = weight_data.get<int>();
    if (weight < 0 || weight > 9999) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.weight = weight;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの売値をセットする
 * @param cost_data cost情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_cost(const nlohmann::json &cost_data, ArtifactType &artifact)
{
    if (cost_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    if (!cost_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto cost = cost_data.get<int>();
    if (cost < 0 || cost > 99999999) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.cost = cost;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトのベースACをセットする
 * @param ac_data ac情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_base_ac(const nlohmann::json &ac_data, ArtifactType &artifact)
{
    if (ac_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!ac_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto base_ac = ac_data.get<short>();
    if (base_ac < -99 || base_ac > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.ac = base_ac;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトのベースダイスをセットする
 * @param dice_data ac情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_base_dice(const nlohmann::json &dice_data, ArtifactType &artifact)
{
    if (!dice_data.is_string()) {
        return PARSE_ERROR_NONE;
    }

    const auto &dice = str_split(dice_data.get<std::string>(), 'd', false, 2);
    if (dice.size() < 2) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    artifact.dd = std::stoi(dice[0]);
    artifact.ds = std::stoi(dice[1]);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの命中補正値をセットする
 * @param to_h_data 命中補正値情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_to_hit(const nlohmann::json &to_h_data, ArtifactType &artifact)
{
    if (to_h_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!to_h_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto to_hit = to_h_data.get<short>();
    if (to_hit < -99 || to_hit > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.to_h = to_hit;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトのダメージ補正値をセットする
 * @param to_d_data ダメージ補正値情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_to_damage(const nlohmann::json &to_d_data, ArtifactType &artifact)
{
    if (to_d_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!to_d_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto to_damage = to_d_data.get<int>();
    if (to_damage < -99 || to_damage > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.to_d = to_damage;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトのAC補正値をセットする
 * @param to_ac_data AC補正値情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_to_ac(const nlohmann::json &to_ac_data, ArtifactType &artifact)
{
    if (to_ac_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!to_ac_data.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto to_ac = to_ac_data.get<short>();
    if (to_ac < -99 || to_ac > 99) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.to_a = to_ac;
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの発動能力をセットする
 * @param act_data 発動能力情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_activate(const nlohmann::json &act_data, ArtifactType &artifact)
{
    if (!act_data.is_string()) {
        return PARSE_ERROR_NONE;
    }

    auto activation = grab_one_activation_flag(act_data.get<std::string>());
    if (activation <= RandomArtActType::NONE) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    artifact.act_idx = activation;
    artifact.flags.set(tr_type::TR_ACTIVATE);
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectからモンスターフラグをセットする
 * @param flag_data モンスターフラグ情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_flags(const nlohmann::json &flag_data, ArtifactType &artifact)
{
    if (flag_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flag_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &flag : flag_data) {
        if (!grab_one_artifact_flag(&artifact, flag.get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトのフレーバーをセットする
 * @param flavor_data フレーバー情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
static errr set_art_flavor(const nlohmann::json &flavor_data, ArtifactType &artifact)
{
    if (flavor_data.is_null()) {
        return PARSE_ERROR_NONE;
    }

#ifdef JP
    const auto &ja_flavor = flavor_data.find("ja");
    if (ja_flavor == flavor_data.end()) {
        return PARSE_ERROR_NONE;
    }
    auto ja_flavor_sys = utf8_to_sys(ja_flavor->get<std::string>());
    if (!ja_flavor_sys) {
        return PARSE_ERROR_INVALID_FLAG;
    }
    artifact.text = std::move(*ja_flavor_sys);
#else
    const auto &en_flavor = flavor_data.find("en");
    if (en_flavor == flavor_data.end()) {
        return PARSE_ERROR_NONE;
    }
    artifact.text = en_flavor->get<std::string>();
#endif

    return PARSE_ERROR_NONE;
}

/*!
 * @brief 固定アーティファクト情報(JSON Object)のパース関数
 * @param art_data 固定アーティファクトデータの格納されたJSON Object
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_artifacts_info(nlohmann::json &art_data, angband_header *)
{
    if (!art_data["id"].is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto int_id = art_data["id"].get<int>();
    if (int_id < error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    const auto artifact_id = i2enum<FixedArtifactId>(int_id);

    error_idx = int_id;
    ArtifactType artifact;
    artifact.flags.set(TR_IGNORE_ACID);
    artifact.flags.set(TR_IGNORE_ELEC);
    artifact.flags.set(TR_IGNORE_FIRE);
    artifact.flags.set(TR_IGNORE_COLD);

    if (auto err = set_art_name(art_data["name"], artifact)) {
        msg_format(_("アーティファクトの名称読込失敗。ID: '%d'。", "Failed to load artifact name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_baseitem(art_data["base_item"], artifact)) {
        msg_format(_("アーティファクトのベースアイテム読込失敗。ID: '%d'。", "Failed to load base item of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_parameter_value(art_data["parameter_value"], artifact)) {
        msg_format(_("アーティファクトのパラメータ値読込失敗。ID: '%d'。", "Failed to load parameter value of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_level(art_data["level"], artifact)) {
        msg_format(_("アーティファクトのレベル読込失敗。ID: '%d'。", "Failed to load level of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_rarity(art_data["rarity"], artifact)) {
        msg_format(_("アーティファクトの希少度読込失敗。ID: '%d'。", "Failed to load rarity of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_weight(art_data["weight"], artifact)) {
        msg_format(_("アーティファクトの重量読込失敗。ID: '%d'。", "Failed to load weight of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_cost(art_data["cost"], artifact)) {
        msg_format(_("アーティファクトの売値読込失敗。ID: '%d'。", "Failed to load cost of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_base_ac(art_data["base_ac"], artifact)) {
        msg_format(_("アーティファクトのベースAC読込失敗。ID: '%d'。", "Failed to load base AC of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_base_dice(art_data["base_dice"], artifact)) {
        msg_format(_("アーティファクトのベースダイス読込失敗。ID: '%d'。", "Failed to load base dice of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_to_hit(art_data["hit_bonus"], artifact)) {
        msg_format(_("アーティファクトの命中補正値読込失敗。ID: '%d'。", "Failed to load hit bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_to_damage(art_data["damage_bonus"], artifact)) {
        msg_format(_("アーティファクトの命中補正値読込失敗。ID: '%d'。", "Failed to load damage bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_to_ac(art_data["ac_bonus"], artifact)) {
        msg_format(_("アーティファクトのAC補正値読込失敗。ID: '%d'。", "Failed to load AC bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_activate(art_data["activate"], artifact)) {
        msg_format(_("アーティファクトの発動能力読込失敗。ID: '%d'。", "Failed to load activate ability of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_flags(art_data["flags"], artifact)) {
        msg_format(_("アーティファクトの能力フラグ読込失敗。ID: '%d'。", "Failed to load ability flags of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = set_art_flavor(art_data["flavor"], artifact)) {
        msg_format(_("アーティファクトのフレーバーテキスト読込失敗。ID: '%d'。", "Failed to load flavor text of artifact. ID: '%d'."), error_idx);
        return err;
    }

    ArtifactList::get_instance().emplace(artifact_id, artifact);
    return PARSE_ERROR_NONE;
}
