#pragma once

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include "util/type-concepts.h"
#include <concepts>
#include <nlohmann/json.hpp>
#include <string_view>
#include <tl/optional.hpp>
#include <utility>

class Dice;

using Range = std::pair<int, int>;

errr info_set_string(const nlohmann::json &json, std::string &data, bool is_required);
errr info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required);
errr info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required);
const nlohmann::json &get_json_value(const nlohmann::json &json, std::string_view key);
errr info_validate_json_array(const nlohmann::json &root, std::string_view key, bool allow_empty = true);

/*!
 * @brief JSON Objectから整数値もしくはenum値を取得する

 * 引数で与えられたJSON Objectから整数値もしくはenum値を取得し、 data に格納する。
 *
 * @param json 整数値が格納されたJSON Object
 * @param data 値を格納する変数への参照
 * @param is_required 必須かどうか。
 * JSON値がnullの場合、必須ならPARSE_ERROR_TOO_FEW_ARGUMENTSを返し、任意なら何もせずに成功する。
 * null以外の非整数値は、必須かどうかによらずPARSE_ERROR_INVALID_TYPEを返す。
 * @param range 取得した値の範囲（両端を含む）を指定する。
 * 格納先の型へ変換する前に検証し、範囲外ならPARSE_ERROR_INVALID_FLAGを返す。
 * 指定しない場合は格納先の型の表現範囲も検証せず、static_castで変換する。
 * エラーまたは任意のnullの場合、dataは変更しない。
 * @return エラーコード
 */
template <IntegralOrEnum T>
errr info_set_integer(const nlohmann::json &json, T &data, bool is_required, tl::optional<Range> range = tl::nullopt)
{
    if (json.is_null()) {
        return is_required ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_NONE;
    }
    if (!json.is_number_integer()) {
        return PARSE_ERROR_INVALID_TYPE;
    }

    // 範囲チェックは格納先の型へ変換する前に行う。変換してから比べると、
    // 格納先の型で表現できない値が切り詰められて範囲内に収まり、チェックをすり抜ける
    // (例: uint8_t への 300 は 44 になり Range(0, 255) を通ってしまう)
    if (json.is_number_unsigned()) {
        const auto value = json.get<nlohmann::json::number_unsigned_t>();
        if (range && (std::cmp_less(value, range->first) || std::cmp_greater(value, range->second))) {
            return PARSE_ERROR_INVALID_FLAG;
        }

        data = static_cast<T>(value);
        return PARSE_ERROR_NONE;
    }

    const auto value = json.get<nlohmann::json::number_integer_t>();
    if (range && (value < range->first || value > range->second)) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    data = static_cast<T>(value);
    return PARSE_ERROR_NONE;
}
