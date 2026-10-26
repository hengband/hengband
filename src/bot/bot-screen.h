/*!
 * @file bot-screen.h
 * @brief 端末の画面バッファをJSONへ変換する処理のヘッダ
 */

#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

class term_win;
struct term_type;

std::string make_bot_screen_line_text(const term_win &win, int y, int width);
nlohmann::json make_bot_screen_json(const term_type &t, bool with_attrs);
