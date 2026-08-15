/*!
 * @file bot-control-server.h
 * @brief 外部プログラムからゲームを操作する制御サーバのヘッダ
 */

#pragma once

/*!
 * @brief 1リクエストで注入できるキーの最大数
 * @details
 * これを1度に積み切れるキーキューを持たない端末では、上限に届く前に
 * 空きが足りずキー列が拒否される。端末側がキューの大きさを決める際の目安。
 */
constexpr auto BOT_CONTROL_MAX_KEYS = 1023;

void init_bot_control_server();
void shutdown_bot_control_server();
