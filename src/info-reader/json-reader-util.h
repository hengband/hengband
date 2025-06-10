#pragma once

#include "external-lib/include-json.h"
#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include <concepts>
#include <tl/optional.hpp>
#include <utility>

class Dice;

using Range = std::pair<int, int>;

template <typename T>
concept IntegralOrEnum = std::integral<T> || std::is_enum_v<T>;

errr info_set_string(const nlohmann::json &json, std::string &data, bool is_required);
errr info_set_dice(const nlohmann::json &json, Dice &dice, bool is_required);
errr info_set_bool(const nlohmann::json &json, bool &bool_value, bool is_required);

/*!
 * @brief JSON Objectから整数値もしくはenum値を取得する

 * 引数で与えられたJSON Objectから整数値もしくはenum値を取得し、 data に格納する。
 *
 * @param json 整数値が格納されたJSON Object
 * @param data 値を格納する変数への参照
 * @param is_required 必須かどうか。
 * 必須でJSON Objectがnullの場合はエラーを返す。必須でない場合は何もせずに終了する。
 * @param range 取得した値の範囲を指定する。範囲外の値が取得された場合はエラーを返す。
 * @return エラーコード
 */
template <IntegralOrEnum T>
errr info_set_integer(const nlohmann::json &json, T &data, bool is_required, tl::optional<Range> range = tl::nullopt)
{
    if (json.is_null()) {
        return is_required ? PARSE_ERROR_TOO_FEW_ARGUMENTS : PARSE_ERROR_NONE;
    }
    if (!json.is_number_integer()) {
        return PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }

    const auto value = json.get<T>();
    if (range && (value < static_cast<T>(range->first) || value > static_cast<T>(range->second))) {
        return PARSE_ERROR_INVALID_FLAG;
    }

    data = value;
    return PARSE_ERROR_NONE;
}
