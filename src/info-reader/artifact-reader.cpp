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
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include <algorithm>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

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

    int int_id;
    if (auto err = info_set_integer(get_json_value(this->art_data, "id"), int_id, true, Range(0, 9999))) {
        return err;
    }
    // 実行時の記録は 1〜件数の ID だけを作るため、1 から欠番なく並んでいることを求める。
    if (const auto previous_id = std::max(error_idx, 0); int_id != previous_id + 1) {
        msg_print(_("アーティファクトIDが連番ではありません。ID: '{}'、直前ID: '{}'。", "Artifact ID is not sequential. ID: '{}', previous ID: '{}'."), int_id, previous_id);
        return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
    }
    const auto artifact_id = i2enum<FixedArtifactId>(int_id);

    error_idx = int_id;
    ArtifactDefinition artifact;
    artifact.flags.set(TR_IGNORE_ACID);
    artifact.flags.set(TR_IGNORE_ELEC);
    artifact.flags.set(TR_IGNORE_FIRE);
    artifact.flags.set(TR_IGNORE_COLD);

    if (const auto result = info_set_string(get_json_value(this->art_data, "name"), artifact.name, true); !result) {
        msg_print(_("アーティファクトの名称読込失敗。ID: '{}'。", "Failed to load artifact name. ID: '{}'."), error_idx);
        return result.error();
    }
    if (const auto result = this->set_art_baseitem(artifact); !result) {
        msg_print(_("アーティファクトのベースアイテム読込失敗。ID: '{}'。", "Failed to load base item of artifact. ID: '{}'."), error_idx);
        return result.error();
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "parameter_value"), artifact.pval, false, Range(-128, 128))) {
        msg_print(_("アーティファクトのパラメータ値読込失敗。ID: '{}'。", "Failed to load parameter value of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "level"), artifact.level, true, Range(0, 128))) {
        msg_print(_("アーティファクトのレベル読込失敗。ID: '{}'。", "Failed to load level of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "rarity"), artifact.rarity, true)) {
        msg_print(_("アーティファクトの希少度読込失敗。ID: '{}'。", "Failed to load rarity of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "weight"), artifact.weight, true, Range(0, 9999))) {
        msg_print(_("アーティファクトの重量読込失敗。ID: '{}'。", "Failed to load weight of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "cost"), artifact.cost, true, Range(0, 99999999))) {
        msg_print(_("アーティファクトの売値読込失敗。ID: '{}'。", "Failed to load cost of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "base_ac"), artifact.ac, false, Range(-99, 99))) {
        msg_print(_("アーティファクトのベースAC読込失敗。ID: '{}'。", "Failed to load base AC of artifact. ID: '{}'."), error_idx);
        return err;
    }
    // セーブではダイスの個数・面数をそれぞれ1バイトで書くため、0〜255に限る。
    if (auto err = info_set_dice(get_json_value(this->art_data, "base_dice"), artifact.damage_dice, false, Range(0, 255))) {
        msg_print(_("アーティファクトのベースダイス読込失敗。ID: '{}'。", "Failed to load base dice of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "hit_bonus"), artifact.to_h, false, Range(-99, 99))) {
        msg_print(_("アーティファクトの命中補正値読込失敗。ID: '{}'。", "Failed to load hit bonus of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "damage_bonus"), artifact.to_d, false, Range(-99, 99))) {
        msg_print(_("アーティファクトのダメージ補正値読込失敗。ID: '{}'。", "Failed to load damage bonus of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (auto err = info_set_integer(get_json_value(this->art_data, "ac_bonus"), artifact.to_a, false, Range(-99, 99))) {
        msg_print(_("アーティファクトのAC補正値読込失敗。ID: '{}'。", "Failed to load AC bonus of artifact. ID: '{}'."), error_idx);
        return err;
    }
    if (const auto result = this->set_art_activate(artifact); !result) {
        msg_print(_("アーティファクトの発動能力読込失敗。ID: '{}'。", "Failed to load activate ability of artifact. ID: '{}'."), error_idx);
        return result.error();
    }
    if (const auto result = this->set_art_flags(artifact); !result) {
        msg_print(_("アーティファクトの能力フラグ読込失敗。ID: '{}'。", "Failed to load ability flags of artifact. ID: '{}'."), error_idx);
        return result.error();
    }
    if (const auto result = info_set_string(get_json_value(this->art_data, "flavor"), artifact.text, false); !result) {
        msg_print(_("アーティファクトのフレーバーテキスト読込失敗。ID: '{}'。", "Failed to load flavor text of artifact. ID: '{}'."), error_idx);
        return result.error();
    }
    // 生成・鑑定では元のベースアイテムを種別・副種別から引き、無ければ例外で落ちるため、読み込み時に確かめる。
    // ベースアイテムはアーティファクトより先に読み込まれる。種別・副種別の検索表は使わず、一覧を直接調べる。
    const auto &baseitems = BaseitemList::get_instance();
    const auto has_baseitem = std::any_of(baseitems.begin(), baseitems.end(), [&artifact](const BaseitemDefinition &baseitem) {
        return baseitem.is_valid() && (baseitem.bi_key == artifact.bi_key);
    });
    if (!has_baseitem) {
        msg_print(_("アーティファクトのベースアイテムが存在しない。ID: '{}'。", "Base item of artifact does not exist. ID: '{}'."), error_idx);
        return PARSE_ERROR_INVALID_VALUE;
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

    msg_print(_("未知の伝説のアイテム・フラグ '{}'。", "Unknown artifact flag '{}'."), what);
    return false;
}

/*!
 * @brief JSON Objectから固定アーティファクトのベースアイテムをセットする
 * @param artifact 保管先のアーティファクト
 * @return 成功時は値なし、失敗時は解析エラー。
 */
tl::expected<void, parse_error_type> ArtifactReader::set_art_baseitem(ArtifactDefinition &artifact) const
{
    const auto &baseitem_data = get_json_value(this->art_data, "base_item");
    if (baseitem_data.is_null()) {
        return tl::unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }

    ItemKindType type_value;
    if (auto err = info_set_integer(get_json_value(baseitem_data, "type_value"), type_value, true, Range(0, 128))) {
        return tl::unexpected(static_cast<parse_error_type>(err));
    }

    int subtype_value;
    if (auto err = info_set_integer(get_json_value(baseitem_data, "subtype_value"), subtype_value, true, Range(0, 128))) {
        return tl::unexpected(static_cast<parse_error_type>(err));
    }

    artifact.bi_key = { type_value, subtype_value };
    return {};
}

/*!
 * @brief JSON Objectから固定アーティファクトの発動能力をセットする
 * @param artifact 保管先のアーティファクト
 * @return 成功時は値なし、失敗時は解析エラー。
 */
tl::expected<void, parse_error_type> ArtifactReader::set_art_activate(ArtifactDefinition &artifact) const
{
    const auto &act_data = get_json_value(this->art_data, "activate");
    if (act_data.is_null()) {
        return {};
    }
    if (!act_data.is_string()) {
        return tl::unexpected(PARSE_ERROR_INVALID_TYPE);
    }

    const auto activation = grab_one_activation_flag(act_data.get<std::string>());
    if (activation <= RandomArtActType::NONE) {
        return tl::unexpected(PARSE_ERROR_INVALID_FLAG);
    }

    artifact.act_idx = activation;
    artifact.flags.set(tr_type::TR_ACTIVATE);
    return {};
}

/*!
 * @brief JSON Objectから固定アーティファクトのフラグをセットする
 * @param artifact 保管先のアーティファクト
 * @return 成功時は値なし、失敗時は解析エラー。
 */
tl::expected<void, parse_error_type> ArtifactReader::set_art_flags(ArtifactDefinition &artifact) const
{
    const auto &flag_data = get_json_value(this->art_data, "flags");
    if (flag_data.is_null()) {
        return {};
    }
    if (!flag_data.is_array()) {
        return tl::unexpected(PARSE_ERROR_TOO_FEW_ARGUMENTS);
    }

    for (auto &flag : flag_data) {
        if (!flag.is_string()) {
            return tl::unexpected(PARSE_ERROR_INVALID_FLAG);
        }
        if (!this->grab_one_artifact_flag(artifact, flag.get<std::string>())) {
            return tl::unexpected(PARSE_ERROR_INVALID_FLAG);
        }
    }
    return {};
}
