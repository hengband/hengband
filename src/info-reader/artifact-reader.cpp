#include "info-reader/artifact-reader.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-effects.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/item-flag-reader.h"
#include "info-reader/item-reader-limits.h"
#include "info-reader/json-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "locale/japanese.h"
#include "locale/language-switcher.h"
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

ArtifactReader::Session ArtifactReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::SEQUENTIAL, { 1, MAX_FIXED_ARTIFACT_ID }, _("アーティファクト", "artifact"), last_id);
}

ArtifactReader::ArtifactReader(const nlohmann::json &art_data, Session &session)
    : art_data(art_data)
    , session(session)
{
}

/*!
 * @brief 固定アーティファクト情報(JSON Object)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 */
tl::expected<void, ParseErrorType> ArtifactReader::read() const
{
    const auto id_result = this->session.read_and_report(get_json_value(this->art_data, "id"));
    if (!id_result) {
        return tl::unexpected(id_result.error());
    }
    const auto int_id = *id_result;
    const auto artifact_id = i2enum<FixedArtifactId>(int_id);

    ArtifactDefinition artifact;
    artifact.flags.set(TR_IGNORE_ACID);
    artifact.flags.set(TR_IGNORE_ELEC);
    artifact.flags.set(TR_IGNORE_FIRE);
    artifact.flags.set(TR_IGNORE_COLD);

    if (const auto result = info_set_string(get_json_value(this->art_data, "name"), artifact.name, true); !result) {
        msg_print(_("アーティファクトの名称読込失敗。", "Failed to load artifact name."));
        return result;
    }
    if (const auto result = this->set_art_baseitem(artifact); !result) {
        msg_print(_("アーティファクトのベースアイテム読込失敗。", "Failed to load base item of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "parameter_value"), artifact.pval, false, Range(-128, 128)); !result) {
        msg_print(_("アーティファクトのパラメータ値読込失敗。", "Failed to load parameter value of artifact."));
        return result;
    }
    // level は生成階 (固定アーティファクトの発動の基本難易度にも使う) で、たどり着ける最深の階 (MAX_DEPTH - 1) までとする。
    if (const auto result = info_set_integer(get_json_value(this->art_data, "level"), artifact.level, true, Range(0, MAX_DEPTH - 1)); !result) {
        msg_print(_("アーティファクトのレベル読込失敗。", "Failed to load level of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "rarity"), artifact.rarity, true); !result) {
        msg_print(_("アーティファクトの希少度読込失敗。", "Failed to load rarity of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "weight"), artifact.weight, true, Range(0, 9999)); !result) {
        msg_print(_("アーティファクトの重量読込失敗。", "Failed to load weight of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "cost"), artifact.cost, true, Range(0, ITEM_COST_MAX)); !result) {
        msg_print(_("アーティファクトの売値読込失敗。", "Failed to load cost of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "base_ac"), artifact.ac, false, Range(0, ITEM_BASE_AC_MAX)); !result) {
        msg_print(_("アーティファクトのベースAC読込失敗。", "Failed to load base AC of artifact."));
        return result;
    }
    // セーブではダイスの個数・面数をそれぞれ1バイトで書くため、0〜255に限る。
    if (const auto result = info_set_dice(get_json_value(this->art_data, "base_dice"), artifact.damage_dice, false, Range(0, 255)); !result) {
        msg_print(_("アーティファクトのベースダイス読込失敗。", "Failed to load base dice of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "hit_bonus"), artifact.to_h, false, Range(-ITEM_BONUS_MAX, ITEM_BONUS_MAX)); !result) {
        msg_print(_("アーティファクトの命中補正値読込失敗。", "Failed to load hit bonus of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "damage_bonus"), artifact.to_d, false, Range(-ITEM_BONUS_MAX, ITEM_BONUS_MAX)); !result) {
        msg_print(_("アーティファクトのダメージ補正値読込失敗。", "Failed to load damage bonus of artifact."));
        return result;
    }
    if (const auto result = info_set_integer(get_json_value(this->art_data, "ac_bonus"), artifact.to_a, false, Range(-ITEM_BONUS_MAX, ITEM_BONUS_MAX)); !result) {
        msg_print(_("アーティファクトのAC補正値読込失敗。", "Failed to load AC bonus of artifact."));
        return result;
    }
    if (const auto result = this->set_art_activate(artifact); !result) {
        msg_print(_("アーティファクトの発動能力読込失敗。", "Failed to load activate ability of artifact."));
        return result;
    }
    if (const auto result = this->set_art_flags(artifact); !result) {
        msg_print(_("アーティファクトの能力フラグ読込失敗。", "Failed to load ability flags of artifact."));
        return result;
    }
    if (const auto result = info_set_string(get_json_value(this->art_data, "flavor"), artifact.text, false); !result) {
        msg_print(_("アーティファクトのフレーバーテキスト読込失敗。", "Failed to load flavor text of artifact."));
        return result;
    }
    // 生成・鑑定では元のベースアイテムを種別・副種別から引き、無ければ例外で落ちるため、読み込み時に確かめる。
    // ベースアイテムはアーティファクトより先に読み込まれる。種別・副種別の検索表は使わず、一覧を直接調べる。
    const auto &baseitems = BaseitemList::get_instance();
    const auto has_baseitem = std::any_of(baseitems.begin(), baseitems.end(), [&artifact](const BaseitemDefinition &baseitem) {
        return baseitem.is_valid() && (baseitem.bi_key == artifact.bi_key);
    });
    if (!has_baseitem) {
        msg_print(_("アーティファクトのベースアイテムが存在しない。", "Base item of artifact does not exist."));
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    ArtifactList::get_instance().emplace(artifact_id, std::move(artifact));
    return {};
}

/*!
 * @brief JSON Objectから固定アーティファクトのベースアイテムをセットする
 * @param artifact 保管先のアーティファクト
 * @return 成功時は値なし、失敗時は解析エラー。
 */
tl::expected<void, ParseErrorType> ArtifactReader::set_art_baseitem(ArtifactDefinition &artifact) const
{
    const auto &baseitem_data = get_json_value(this->art_data, "base_item");
    if (baseitem_data.is_null()) {
        return tl::unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
    }

    // 失敗時は直後に返るが、gcc の最適化で未初期化と誤検出されないよう初期化しておく。
    ItemKindType type_value{};
    if (const auto result = info_set_integer(get_json_value(baseitem_data, "type_value"), type_value, true, Range(0, ITEM_TYPE_MAX)); !result) {
        return result;
    }

    int subtype_value = 0;
    if (const auto result = info_set_integer(get_json_value(baseitem_data, "subtype_value"), subtype_value, true, Range(0, item_subtype_max({ type_value, 0 }))); !result) {
        return result;
    }

    artifact.bi_key = { type_value, subtype_value };
    return {};
}

/*!
 * @brief JSON Objectから固定アーティファクトの発動能力をセットする
 * @param artifact 保管先のアーティファクト
 * @return 成功時は値なし、失敗時は解析エラー。
 */
tl::expected<void, ParseErrorType> ArtifactReader::set_art_activate(ArtifactDefinition &artifact) const
{
    const auto activation = read_activation(get_json_value(this->art_data, "activate"));
    if (!activation) {
        return tl::unexpected(activation.error());
    }
    if (*activation == RandomArtActType::NONE) {
        return {};
    }

    artifact.act_idx = *activation;
    artifact.flags.set(tr_type::TR_ACTIVATE);
    return {};
}

/*!
 * @brief JSON Objectから固定アーティファクトのフラグをセットする
 * @param artifact 保管先のアーティファクト
 * @return 成功時は値なし、失敗時は解析エラー。
 */
tl::expected<void, ParseErrorType> ArtifactReader::set_art_flags(ArtifactDefinition &artifact) const
{
    const auto &flag_data = get_json_value(this->art_data, "flags");
    if (flag_data.is_null()) {
        return {};
    }
    return info_read_flag_array(flag_data, [&artifact](const auto &flag) {
        const auto what = flag.template get<std::string>();
        return grab_one_item_flag_or_report(artifact.flags, artifact.gen_flags, what, _("未知の伝説のアイテム・フラグ '{}'。", "Unknown artifact flag '{}'."));
    });
}
