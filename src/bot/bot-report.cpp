/*!
 * @file bot-report.cpp
 * @brief 制御サーバの診断メッセージを出力する処理の実装
 * @details
 * 制御サーバの診断はplog()へ流さない。Windows版のGUIではplog_auxが
 * メッセージボックスの表示になっており、待ち受けの開始を知らせるだけで
 * モーダルダイアログが出てゲームが止まってしまうためである。
 * 外部プログラムから観測することを想定した情報なので、標準エラー出力へ出す。
 */

#include "bot/bot-report.h"
#include "system/angband.h"
#include <cstdio>
#include <fmt/format.h>

#ifdef WINDOWS
#include "main-win/main-win-utils.h"
#endif

/*!
 * @brief 診断メッセージの出力先を準備する
 * @details
 * Windowsアプリケーションは既定でコンソールを持たず、標準エラー出力が
 * どこにも繋がっていない。制御サーバを起動する際に一度だけ呼ぶこと。
 */
void prepare_bot_report()
{
#ifdef WINDOWS
    attach_console();
#endif
}

/*!
 * @brief 診断メッセージを標準エラー出力へ出力する
 * @param message 出力するメッセージ
 */
void report_bot_message(std::string_view message)
{
    fmt::println(stderr, "bot-control: {}", message);
    std::fflush(stderr);
}
