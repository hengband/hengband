#pragma once
/*!
 * @file commandline-win.h
 * @brief Windows版固有実装(コマンドライン)ヘッダ
 */

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
     * --output-spoilers | 全スポイラー出力を行う
     */
    void handle(void);
};

/*!
 * コマンドライン情報管理
 */
extern CommandLine command_line;

extern void create_debug_spoiler(void);
