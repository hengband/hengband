/*!
 * @file bot-control-server.h
 * @brief 外部プログラムからゲームを操作する制御サーバのヘッダ
 */

#pragma once

#include <string>
#include <tl/expected.hpp>

struct TermSize;

/*!
 * @brief 1リクエストで注入できるキーの最大数
 * @details
 * これを1度に積み切れるキーキューを持たない端末では、上限に届く前に
 * 空きが足りずキー列が拒否される。端末側がキューの大きさを決める際の目安。
 */
constexpr auto BOT_CONTROL_MAX_KEYS = 1023;

/*!
 * @brief 端末の大きさを変える関数
 * @details 引数は angband_terms 上の添字と新しい大きさ。失敗した場合は理由を返す。
 */
using BotTermResizer = tl::expected<void, std::string> (*)(int index, const TermSize &size);

void set_bot_term_resizer(BotTermResizer resizer);
void init_bot_control_server();
void shutdown_bot_control_server();
