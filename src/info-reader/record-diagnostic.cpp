#include "info-reader/record-diagnostic.h"
#include "info-reader/json-reader-util.h"
#include "locale/language-switcher.h"
#include <algorithm>
#include <fmt/format.h>

RecordDiagnostic::RecordDiagnostic(DefinitionIdChecker &session, IntegerDiagnostic integer_diagnostic)
    : session(session)
    , integer_diagnostic(integer_diagnostic)
{
}

/*!
 * @brief 診断を初期化し、今回のレコードの識別子表記を保持する。
 */
void RecordDiagnostic::begin_read(const nlohmann::json &data)
{
    this->diagnostic.reset();
    this->record_id = data.is_object() && data.contains("id") ? data["id"].dump() : "<unknown>";
}

const std::optional<RecordReadError> &RecordDiagnostic::error() const
{
    return this->diagnostic;
}

tl::expected<void, ParseErrorType> RecordDiagnostic::fail(ParseErrorType code, std::string_view path, std::string_view reason)
{
    this->diagnostic = RecordReadError{ this->record_id, std::string(path), std::string(reason) };
    return tl::unexpected(code);
}

/*!
 * @brief 必須項目を指定順に確認してから未知の項目を検出する。
 * @details ルートの id 欠落は各定義の ID 検証器の診断を使用する。
 */
tl::expected<void, ParseErrorType> RecordDiagnostic::check_keys(const nlohmann::json &data, std::string_view path, std::span<const std::string_view> keys)
{
    if (!data.is_object()) {
        return this->fail(ParseErrorType::INVALID_TYPE, path, _("オブジェクトが必要です", "expected an object"));
    }
    for (const auto key : keys) {
        if (!data.contains(key)) {
            if (path == "$" && key == "id") {
                const auto result = this->session.read(get_json_value(data, key));
                return this->fail(result.error().code, "$.id", result.error().reason);
            }
            return this->fail(ParseErrorType::TOO_FEW_ARGUMENTS, fmt::format("{}.{}", path, key), _("必須項目がありません", "missing required field"));
        }
    }
    for (const auto &key : data.items()) {
        if (std::find(keys.begin(), keys.end(), key.key()) == keys.end()) {
            return this->fail(ParseErrorType::UNDEFINED_DIRECTIVE, fmt::format("{}.{}", path, key.key()), _("未知の項目です", "unknown field"));
        }
    }
    return {};
}

/*!
 * @brief 整数と範囲を検証し、従来の Reader ごとの診断文言を保持する。
 */
tl::expected<void, ParseErrorType> RecordDiagnostic::read_integer(const nlohmann::json &data, int &value, int minimum, int maximum, std::string_view path)
{
    if (const auto result = info_set_integer(data, value, true, Range(minimum, maximum)); !result) {
        const auto reason = this->integer_diagnostic == IntegerDiagnostic::ALWAYS_RANGE || data.is_number_integer()
                                ? fmt::format(_("{}以上{}以下の整数が必要です", "expected an integer in [{}, {}]"), minimum, maximum)
                                : _("整数が必要です", "expected an integer");
        return this->fail(result.error(), path, reason);
    }
    return {};
}
