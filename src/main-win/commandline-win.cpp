/*!
 * @file commandline-win.cpp
 * @brief Windows版固有実装(コマンドライン)
 */

#include "main-win/commandline-win.h"
#include "main-win/main-win-utils.h"
#include "term/z-util.h"

#include <iostream>
#include <windows.h>

// interface object
CommandLine command_line{};

namespace {
// セーブファイル名
std::string savefile_option;
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
            fwprintf(stdout, L"argv[%d] : %s\n", i, argv[i]);
            if (wcscmp(argv[i], L"--debug-console") == 0) {
                create_console();
                continue;
            } else if (wcscmp(argv[i], L"--output-spoilers") == 0) {
                create_debug_spoiler();
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
        quit(nullptr);
    }
}

const std::string &CommandLine::get_savefile_option(void)
{
    return savefile_option;
}
