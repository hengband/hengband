#pragma once
/*!
 * @file parse-error-messages.h
 * @brief 変愚蛮怒のゲームデータ解析エラー名ヘッダ
 */

#include "info-reader/parse-error-types.h"
#include "system/angband.h"
#include "util/enum-converter.h"
#include <cstddef>
#include <string>
#include <string_view>

extern concptr err_str[enum2i(ParseErrorType::MAX)];

std::string_view describe_parse_error(ParseErrorType err);
std::string format_record_parse_error(std::string_view filename, std::string_view keyname, size_t index, ParseErrorType err);
std::string format_json_syntax_error(std::string_view filename, std::string_view what);
