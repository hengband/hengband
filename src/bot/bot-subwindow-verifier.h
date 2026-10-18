/*!
 * @file bot-subwindow-verifier.h
 * @brief サブウィンドウの描き直しの漏れを調べる処理のヘッダ
 */

#pragma once

#include <nlohmann/json_fwd.hpp>

class PlayerType;

nlohmann::json verify_bot_subwindows(PlayerType *player_ptr);
