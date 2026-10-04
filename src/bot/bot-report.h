/*!
 * @file bot-report.h
 * @brief 制御サーバの診断メッセージを出力する処理のヘッダ
 */

#pragma once

#include <string_view>

void prepare_bot_report();
void report_bot_message(std::string_view message);
