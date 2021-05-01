/*!
 * @file commandline-win.cpp
 * @brief Windows版固有実装(コマンドライン)
 */

#include "main-win/commandline-win.h"
#include "term/z-util.h"

#include <windows.h>

// interface object
CommandLine command_line{};

void CommandLine::handle(void)
{
    int argc;
    LPWSTR *argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (wcscmp(argv[i], L"--output-spoilers") == 0) {
                create_debug_spoiler();
                continue;
            }
        }

        ::LocalFree(argv);
    } else {
        fprintf(stdout, "CommandLineToArgvW failed.");
        quit(NULL);
    }
}
