/*!
 * @file commandline-win.cpp
 * @brief Windows版固有実装(コマンドライン)
 */

#include "main-win/commandline-win.h"
#include "game-option/runtime-arguments.h"
#include "main-win/main-win-utils.h"
#include "term/z-util.h"

#include <fmt/format.h>
#include <iostream>
#include <string>
#include <string_view>
#include <windows.h>

// interface object
CommandLine command_line{};

namespace {
// セーブファイル名
std::string savefile_option;

/*!
 * @brief プラットフォームに依存しない長いコマンドライン引数を解釈する
 * @param option コマンドライン引数
 * @return 実行時オプションとして解釈した場合TRUE
 * @details
 * オプションの綴りと値の解釈をUnix版と共有するため、ワイド文字列をマルチバイトへ変換して
 * parse_runtime_argument()に委譲する。値が不正な場合はその旨を通知して終了する。
 */
bool parse_runtime_option(const WCHAR *option)
{
    auto converted = to_multibyte(option);
    if (converted.c_str() == nullptr) {
        return false;
    }

    const std::string_view narrow_option(converted.c_str());
    if (!narrow_option.starts_with("--")) {
        return false;
    }

    switch (parse_runtime_argument(narrow_option.substr(2))) {
    case RuntimeArgumentResult::HANDLED:
        return true;
    case RuntimeArgumentResult::INVALID:
        quit(fmt::format("Invalid value in '{}'", narrow_option));
        return true;
    case RuntimeArgumentResult::NOT_HANDLED:
        return false;
    }

    return false;
}
}

/*!
 * @brief コンソールを作成する
 * @details
 * 標準出力のみ対応。
 */
static void create_console(void)
{
    ::AllocConsole();
    FILE *stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    std::cout << "Hengband debug console" << std::endl;
}

void CommandLine::handle(void)
{
    int argc;
    LPWSTR *argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; i++) {
            // 引数ダンプは診断用。--bot-json-output=- は標準出力をJSONLの排他的な
            // 出力先として使うため、標準エラーへ出す
            fwprintf(stderr, L"argv[%d] : %s\n", i, argv[i]);
            if (wcscmp(argv[i], L"-o") == 0) {
                arg_force_original = true;
                continue;
            } else if (wcscmp(argv[i], L"-r") == 0) {
                arg_force_roguelike = true;
                continue;
            } else if (wcscmp(argv[i], L"--debug-console") == 0) {
                create_console();
                continue;
            } else if (wcscmp(argv[i], L"--output-spoilers") == 0) {
                create_debug_spoiler();
                continue;
            } else if (parse_runtime_option(argv[i])) {
                continue;
            } else {
                if (argv[i][0] != L'-') {
                    // "-"で始まらない最初のオプションをセーブファイル名とみなす
                    if (savefile_option.empty()) {
                        savefile_option = to_multibyte(argv[i]).c_str();
                    }
                }
            }
        }

        ::LocalFree(argv);
    } else {
        fprintf(stdout, "CommandLineToArgvW failed.");
        quit("");
    }
}

const std::string &CommandLine::get_savefile_option(void)
{
    return savefile_option;
}
