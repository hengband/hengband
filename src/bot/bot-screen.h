/*!
 * @file bot-screen.h
 * @brief 端末の画面バッファをJSONへ変換する処理のヘッダ
 */

#pragma once

#include <nlohmann/json_fwd.hpp>

struct term_type;

nlohmann::json make_bot_screen_json(const term_type &t, bool with_attrs);
