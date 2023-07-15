#pragma once
/*!
 * @file commandline-win.h
 * @brief Windows版固有実装(コマンドライン)ヘッダ
 */

#include <string>

/*!
 * @brief コマンドライン情報管理
 */
class CommandLine {
public:
    CommandLine() = default;
    ~CommandLine() = default;
    void operator=(const CommandLine &) = delete;

    /*!
     * @brief コマンドラインオプション処理
     * @details
     * コマンドラインオプションは以下の通り。
     * オプション | 内容
     * ---------- | ----
     * --debug-console | デバッグ用コンソール表示を行う
     * --output-spoilers | 全スポイラー出力を行う
     * 「-」で始まらない最初のオプション | セーブファイルパス
     */
    void handle(void);

    /*!
     * @brief セーブファイル名取得
     * @details 事前にhandle()を呼び出していること。
     * @return コマンドラインで指定されているセーブファイル名。設定されていなければ空文字列を返す。
     */
    const std::string &get_savefile_option(void);
};

/*!
 * コマンドライン情報管理
 */
extern CommandLine command_line;

extern void create_debug_spoiler(void);
