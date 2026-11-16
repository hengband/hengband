/*!
 * @file player-history.h
 * @brief プレイヤーの生い立ちの型と大きさの定義
 */

#pragma once

#include <array>
#include <string>
#include <string_view>

constexpr auto PLAYER_HISTORY_LINES = 4; //!< 生い立ちの行数
constexpr auto PLAYER_HISTORY_WIDTH = 59; //!< 生い立ちの1行の表示幅の上限

//! 生い立ち (1 行ごとの文字列)
using PlayerHistory = std::array<std::string, PLAYER_HISTORY_LINES>;

PlayerHistory shape_player_history(std::string_view text);
