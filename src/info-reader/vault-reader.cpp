#include "info-reader/vault-reader.h"
#include "floor/floor-base-definitions.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/json-reader-util.h"
#include "locale/character-encoding.h"
#include "locale/language-switcher.h"
#include "room/rooms-vault.h"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <limits>
#include <utility>

VaultReader::Session VaultReader::create_session(tl::optional<int> last_id)
{
    return Session(DefinitionIdOrder::STRICTLY_INCREASING, { 0, std::numeric_limits<short>::max() - 1 }, _("Vault", "vault"), last_id);
}

VaultReader::VaultReader(const nlohmann::json &data, Session &session)
    : RecordDiagnostic(session)
    , data(data)
{
}

/*!
 * @brief Vault定義(JSON Object)のパース関数
 * @return 成功時は空の値、失敗時は解析エラーコード
 * @details 失敗時の診断は error() で取得できる。
 */
tl::expected<void, ParseErrorType> VaultReader::read()
{
    this->begin_read(this->data);
    constexpr auto keys = std::to_array<std::string_view>({ "id", "name", "type", "rating", "height", "width", "layout" });
    if (const auto result = this->check_keys(this->data, "$", keys); !result) {
        return result;
    }
    const auto id_result = this->session.read(get_json_value(this->data, "id"));
    if (!id_result) {
        return this->fail(id_result.error().code, "$.id", id_result.error().reason);
    }
    const auto id = *id_result;

    vault_type vault;
    vault.idx = static_cast<short>(id);
    if (!this->data["name"].is_string() || this->data["name"].get_ref<const std::string &>().empty()) {
        return this->fail(ParseErrorType::INVALID_TYPE, "$.name", _("空でない文字列が必要です", "expected a nonempty string"));
    }
    auto name = utf8_to_sys(this->data["name"].get_ref<const std::string &>());
    if (!name) {
        return this->fail(ParseErrorType::INVALID_VALUE, "$.name", _("システムの文字コードに変換できません", "cannot be converted to the system encoding"));
    }
    vault.name = std::move(*name);
    int type;
    if (const auto result = this->read_integer(this->data["type"], type, 0, 255, "$.type"); !result) {
        return result;
    }
    if (type != 7 && type != 8 && type != 17) {
        return this->fail(ParseErrorType::INVALID_VALUE, "$.type", _("Vault種別は7、8、17のいずれかです", "vault type must be 7, 8 or 17"));
    }
    vault.typ = static_cast<uint8_t>(type);
    // rating はゲーム中で参照しない（未使用）。定義の形を保つため、読み込んで保持だけする。
    if (const auto result = this->read_integer(this->data["rating"], vault.rat, 0, std::numeric_limits<int>::max(), "$.rating"); !result) {
        return result;
    }
    if (const auto result = this->read_integer(this->data["height"], vault.hgt, 1, MAX_HGT, "$.height"); !result) {
        return result;
    }
    if (const auto result = this->read_integer(this->data["width"], vault.wid, 1, MAX_WID, "$.width"); !result) {
        return result;
    }
    const auto &layout = this->data["layout"];
    if (!layout.is_array() || layout.size() != static_cast<size_t>(vault.hgt)) {
        return this->fail(ParseErrorType::INVALID_VALUE, "$.layout", _("行数がheightと一致しません", "row count must equal height"));
    }
    for (size_t i = 0; i < layout.size(); ++i) {
        const auto path = fmt::format("$.layout[{}]", i);
        if (!layout[i].is_string()) {
            return this->fail(ParseErrorType::INVALID_TYPE, path, _("文字列が必要です", "expected a string"));
        }
        const auto &row = layout[i].get_ref<const std::string &>();
        if (row.size() != static_cast<size_t>(vault.wid)) {
            return this->fail(ParseErrorType::INVALID_VALUE, path, _("行のバイト数がwidthと一致しません", "row byte length must equal width"));
        }
        if (std::any_of(row.begin(), row.end(), [](unsigned char c) { return c < 0x20 || c > 0x7e; })) {
            return this->fail(ParseErrorType::INVALID_VALUE, path, _("配置図には印字可能なASCII文字を指定してください", "layout must contain printable ASCII characters"));
        }
        vault.text += row;
    }
    // 検証を完了したレコードだけを公開し、失敗時は既存の状態を保持する。
    if (static_cast<size_t>(id) >= vaults_info.size()) {
        vaults_info.resize(id + 1);
    }
    vaults_info[id] = std::move(vault);
    return {};
}
