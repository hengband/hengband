/*!
 * @file bot-screen.h
 * @brief 端末の画面バッファをJSONへ変換する処理のヘッダ
 */

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

struct term_type;

std::string to_json_utf8(std::string_view str);
nlohmann::json make_bot_screen_json(const term_type &t, bool with_attrs);
