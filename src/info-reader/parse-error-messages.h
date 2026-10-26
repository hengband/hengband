#pragma once
/*!
 * @file parse-error-messages.h
 * @brief 変愚蛮怒のゲームデータ解析エラー名ヘッダ
 */

#include "info-reader/parse-error-types.h"
#include <cstddef>
#include <nlohmann/json_fwd.hpp>
#include <span>
#include <string>
#include <string_view>

/*!
 * @brief 定義の要素を見分けるための項目 (キーの名前と、要素の中の値を指すポインタ)
 * @details value が nullptr なら識別子は無い。指す先は要素が生きている間だけ有効。
 */
struct RecordIdentifier {
    std::string_view key;
    const nlohmann::json *value = nullptr;
};

std::string_view describe_parse_error(ParseErrorType err);
std::string format_record_parse_error(std::string_view filename, std::string_view keyname, size_t index, std::string_view identifier, ParseErrorType err);
RecordIdentifier find_record_identifier(const nlohmann::json &record);
std::string describe_record_identifier(const RecordIdentifier &identifier);
std::string format_json_syntax_error(std::string_view filename, std::string_view what);
std::string format_definition_load_error(std::span<const std::string> diagnostics, std::string_view error);
