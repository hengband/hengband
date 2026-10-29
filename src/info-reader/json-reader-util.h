#pragma once

#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/range.h"
#include "system/angband.h"
#include "util/type-concepts.h"
#include <concepts>
#include <functional>
#include <nlohmann/json.hpp>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <type_traits>
#include <utility>

class Dice;

tl::expected<void, ParseErrorType> info_read_flag_array(const nlohmann::json &json, const std::function<bool(const nlohmann::json &)> &grab_flag);

tl::expected<void, ParseErrorType> info_set_string(const nlohmann::json &json, std::string &data, bool is_required);
tl::expected<void, ParseErrorType> info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required, tl::optional<Range> range = tl::nullopt);
tl::expected<void, ParseErrorType> info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required);
const nlohmann::json &get_json_value(const nlohmann::json &json, std::string_view key);
tl::expected<void, ParseErrorType> info_validate_json_array(const nlohmann::json &root, std::string_view key, bool allow_empty = true);

/*!
 * @brief JSONのトークンを辞書または検索関数で値へ変換する
 * @param json トークン文字列。nullは必須値の欠落として扱う
 * @param table 辞書、または文字列を受け取りoptionalを返す検索関数
 * @param type_error 文字列以外の値に対する互換用エラーコード
 * @return 対応する値。欠落、型違い、未知の名前はそれぞれ解析エラーを返す
 */
template <typename Table>
auto info_read_token(const nlohmann::json &json, const Table &table, ParseErrorType type_error = ParseErrorType::INVALID_TYPE)
{
    const auto lookup = [&table](const std::string &name) {
        if constexpr (std::is_invocable_v<const Table &, const std::string &>) {
            return table(name);
        } else {
            return info_get_const(table, name);
        }
    };
    using Value = typename decltype(lookup(std::declval<const std::string &>()))::value_type;
    using Result = tl::expected<Value, ParseErrorType>;
    if (json.is_null()) {
        return Result(tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS));
    }
    if (!json.is_string()) {
        return Result(tl::make_unexpected(type_error));
    }
    const auto value = lookup(json.get_ref<const std::string &>());
    if (!value) {
        return Result(tl::make_unexpected(ParseErrorType::INVALID_FLAG));
    }
    return Result(*value);
}

/*!
 * @brief JSON Objectから整数値もしくはenum値を取得する

 * 引数で与えられたJSON Objectから整数値もしくはenum値を取得し、 data に格納する。
 *
 * @param json 整数値が格納されたJSON Object
 * @param data 値を格納する変数への参照
 * @param is_required 必須かどうか。
 * JSON値がnullの場合、必須ならParseErrorType::TOO_FEW_ARGUMENTSを返し、任意なら何もせずに成功する。
 * null以外の非整数値は、必須かどうかによらずParseErrorType::INVALID_TYPEを返す。
 * @param range 取得した値の範囲（両端を含む）を指定する。
 * 格納先の型へ変換する前に検証し、範囲外ならParseErrorType::INVALID_VALUEを返す。
 * range の指定にかかわらず格納先の型 (enum は基底型) の表現範囲も検証する。
 * エラーまたは任意のnullの場合、dataは変更しない。
 * @return 成功時は空の値、失敗時は解析エラーコード（ParseErrorType::TOO_FEW_ARGUMENTS、ParseErrorType::INVALID_TYPE、
 * ParseErrorType::INVALID_VALUEのいずれか。条件は上記のとおり）
 */
template <IntegralOrEnum T>
tl::expected<void, ParseErrorType> info_set_integer(const nlohmann::json &json, T &data, bool is_required, tl::optional<Range> range = tl::nullopt)
{
    if (json.is_null()) {
        if (is_required) {
            return tl::make_unexpected(ParseErrorType::TOO_FEW_ARGUMENTS);
        }
        return {};
    }
    if (!json.is_number_integer()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    // 範囲チェックは格納先の型へ変換する前に行う。変換してから比べると、
    // 格納先の型で表現できない値が切り詰められて範囲内に収まり、チェックをすり抜ける
    // (例: uint8_t への 300 は 44 になり Range(0, 255) を通ってしまう)
    const auto fits_destination = [](auto value) {
        if constexpr (std::is_enum_v<T>) {
            return std::in_range<std::underlying_type_t<T>>(value);
        } else {
            return std::in_range<T>(value);
        }
    };
    if (json.is_number_unsigned()) {
        const auto value = json.get<nlohmann::json::number_unsigned_t>();
        if (!fits_destination(value) || (range && (std::cmp_less(value, range->first) || std::cmp_greater(value, range->second)))) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }

        data = static_cast<T>(value);
        return {};
    }

    const auto value = json.get<nlohmann::json::number_integer_t>();
    if (!fits_destination(value) || (range && (value < range->first || value > range->second))) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }

    data = static_cast<T>(value);
    return {};
}

tl::expected<void, ParseErrorType> info_validate_version(const nlohmann::json &root, int supported);
