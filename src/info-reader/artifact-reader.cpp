#include "info-reader/artifact-reader.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "info-reader/baseitem-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/japanese.h"
#include "object-enchant/tr-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

ArtifactReader::ArtifactReader(const nlohmann::json &art_data)
    : art_data(art_data)
{
}

/*!
 * @brief 固定アーティファクト情報(JSON Object)のパース関数
 * @param art_data 固定アーティファクトデータの格納されたJSON Object
 * @return エラーコード
 */
int ArtifactReader::read() const
{
    if (!get_json_value(this->art_data, "id").is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto int_id = get_json_value(this->art_data, "id").get<int>();
    if (int_id <= error_idx) {
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    const auto artifact_id = i2enum<FixedArtifactId>(int_id);

    error_idx = int_id;
    ArtifactDefinition artifact;
    artifact.flags.set(TR_IGNORE_ACID);
    artifact.flags.set(TR_IGNORE_ELEC);
    artifact.flags.set(TR_IGNORE_FIRE);
    artifact.flags.set(TR_IGNORE_COLD);

    if (auto err = info_set_string(get_json_value(this->art_data, "name"), artifact.name, true)) {
        msg_format(_("アーティファクトの名称読込失敗。ID: '%d'。", "Failed to load artifact name. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = this->set_art_baseitem(artifact)) {
        msg_format(_("アーティファクトのベースアイテム読込失敗。ID: '%d'。", "Failed to load base item of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "parameter_value"), artifact.pval, false, Range(-128, 128))) {
        msg_format(_("アーティファクトのパラメータ値読込失敗。ID: '%d'。", "Failed to load parameter value of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "level"), artifact.level, true, Range(0, 128))) {
        msg_format(_("アーティファクトのレベル読込失敗。ID: '%d'。", "Failed to load level of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "rarity"), artifact.rarity, true)) {
        msg_format(_("アーティファクトの希少度読込失敗。ID: '%d'。", "Failed to load rarity of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "weight"), artifact.weight, true, Range(0, 9999))) {
        msg_format(_("アーティファクトの重量読込失敗。ID: '%d'。", "Failed to load weight of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "cost"), artifact.cost, true, Range(0, 99999999))) {
        msg_format(_("アーティファクトの売値読込失敗。ID: '%d'。", "Failed to load cost of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "base_ac"), artifact.ac, false, Range(-99, 99))) {
        msg_format(_("アーティファクトのベースAC読込失敗。ID: '%d'。", "Failed to load base AC of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_dice(get_json_value(this->art_data, "base_dice"), artifact.damage_dice, false)) {
        msg_format(_("アーティファクトのベースダイス読込失敗。ID: '%d'。", "Failed to load base dice of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "hit_bonus"), artifact.to_h, false, Range(-99, 99))) {
        msg_format(_("アーティファクトの命中補正値読込失敗。ID: '%d'。", "Failed to load hit bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "damage_bonus"), artifact.to_d, false, Range(-99, 99))) {
        msg_format(_("アーティファクトのダメージ補正値読込失敗。ID: '%d'。", "Failed to load damage bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "ac_bonus"), artifact.to_a, false, Range(-99, 99))) {
        msg_format(_("アーティファクトのAC補正値読込失敗。ID: '%d'。", "Failed to load AC bonus of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = this->set_art_activate(artifact)) {
        msg_format(_("アーティファクトの発動能力読込失敗。ID: '%d'。", "Failed to load activate ability of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = this->set_art_flags(artifact)) {
        msg_format(_("アーティファクトの能力フラグ読込失敗。ID: '%d'。", "Failed to load ability flags of artifact. ID: '%d'."), error_idx);
        return err;
    }
    if (auto err = info_set_string(get_json_value(this->art_data, "flavor"), artifact.text, false)) {
        msg_format(_("アーティファクトのフレーバーテキスト読込失敗。ID: '%d'。", "Failed to load flavor text of artifact. ID: '%d'."), error_idx);
        return err;
    }

    ArtifactList::get_instance().emplace(artifact_id, std::move(artifact));
    return PARSE_ERROR_NONE;
}

/*!
 * @brief テキストトークンを走査してフラグを一つ得る(アーティファクト用) /
 * Grab one activation index flag
 * @param a_ptr 保管先のアーティファクト構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つかったらtrue
 */
bool ArtifactReader::grab_one_artifact_flag(ArtifactDefinition &artifact, std::string_view what) const
{
    if (TrFlags::grab_one_flag(artifact.flags, baseitem_flags, what)) {
        return true;
    }

    if (EnumClassFlagGroup<ItemGenerationTraitType>::grab_one_flag(artifact.gen_flags, baseitem_geneneration_flags, what)) {
        return true;
    }

    msg_format(_("未知の伝説のアイテム・フラグ '%s'。", "Unknown artifact flag '%s'."), what.data());
    return false;
}

/*!
 * @brief JSON Objectから固定アーティファクトのベースアイテムをセットする
 * @param baseitem_data ベースアイテム情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
int ArtifactReader::set_art_baseitem(ArtifactDefinition &artifact) const
{
    const auto &baseitem_data = get_json_value(this->art_data, "base_item");
    if (baseitem_data.is_null()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    ItemKindType type_value;
    if (auto err = info_set_integer(get_json_value(baseitem_data, "type_value"), type_value, true, Range(0, 128))) {
        return err;
    }

    int subtype_value;
    if (auto err = info_set_integer(get_json_value(baseitem_data, "subtype_value"), subtype_value, true, Range(0, 128))) {
        return err;
    }

    artifact.bi_key = { type_value, subtype_value };
    return PARSE_ERROR_NONE;
}

/*!
 * @brief JSON Objectから固定アーティファクトの発動能力をセットする
 * @param act_data 発動能力情報の格納されたJSON Object
 * @param artifact 保管先のアーティファクト
 * @return エラーコード
 */
int ArtifactReader::set_art_activate(ArtifactDefinition &artifact) const
{
    const auto &act_data = get_json_value(this->art_data, "activate");
    if (act_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!act_data.is_string()) {
        return PARSE_ERROR_INVALID_TYPE;
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
int ArtifactReader::set_art_flags(ArtifactDefinition &artifact) const
{
    const auto &flag_data = get_json_value(this->art_data, "flags");
    if (flag_data.is_null()) {
        return PARSE_ERROR_NONE;
    }
    if (!flag_data.is_array()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    for (auto &flag : flag_data) {
        if (!flag.is_string()) {
            return PARSE_ERROR_INVALID_FLAG;
        }
        if (!this->grab_one_artifact_flag(artifact, flag.get<std::string>())) {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    return PARSE_ERROR_NONE;
}
