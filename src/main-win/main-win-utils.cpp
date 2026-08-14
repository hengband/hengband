/*!
 * @file main-win-utils.cpp
 * @brief Windows版固有実装(ユーティリティー)
 */

#include "main-win/main-win-utils.h"
#include "cmd-io/cmd-process-screen.h"
#include "locale/language-switcher.h"
#include "main-win/main-win-define.h"
#include "system/angband-version.h"
#include <cstdio>
#include <string>

/*!
 * @brief (Windows固有)変愚蛮怒が起動済かどうかのチェック
 * @details
 * 特定の名前のミューテックスオブジェクトの所有権取得を試みる。
 * 取得できない場合は他に変愚蛮怒のプロセスが起動しているとみなす。
 * 取得したミューテックスのハンドルは明示的な解放は行わず、プロセス終了時にOSが解放する。
 * バリアント名の先頭から63文字が一致していたらそれらを同時起動できない (が、そのようなことは起きない想定)。
 * @return 起動済の変愚蛮怒プロセス有無
 */
bool is_already_running(void)
{
    constexpr auto max_mutex_length = 64;
    wchar_t wtext[max_mutex_length]{};
    mbstowcs(wtext, VARIANT_NAME.data(), max_mutex_length - 1);
    [[maybe_unused]] HANDLE hMutex = CreateMutexW(NULL, TRUE, wtext);
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

/*!
 * @brief (Windows固有)標準エラー出力を確保する
 * @details
 * Windowsアプリケーションは既定でコンソールを持たないため、
 * 診断メッセージを観測できるようコンソールを割り当てて標準エラー出力を接続する。
 * 親プロセスのコンソールがあればそちらを優先して使う。
 * 既に接続済みの場合に呼んでも問題は無い。
 *
 * ただし「Hengband.exe --control-port=9000 2>err.log」のように標準エラー出力の
 * 行き先が明示的に指定されている場合、コンソールで置き換えると診断がログに残らなくなる。
 * 自動化からログを取る運用を妨げないよう、既に有効なハンドルがあればそちらを尊重する。
 * この判定はAttachConsole()/AllocConsole()より前に行う必要がある。
 * これらのAPIはコンソールを持たないプロセスに対して標準ハンドル自体を新規に有効化してしまうため、
 * 呼び出し後に判定すると常に「既に接続済み」と誤認し、CRTのstderr用FILE*が
 * CONOUT$へ束縛されないままになる(Win32の標準ハンドルとCRTのFILE*束縛は別物)。
 * コンソールの確保自体は、create_console()が標準出力をCONOUT$へ繋ぐ前提となっているため常に行う。
 */
void attach_console()
{
    const auto handle = ::GetStdHandle(STD_ERROR_HANDLE);
    const auto has_redirected_stderr = (handle != nullptr) && (handle != INVALID_HANDLE_VALUE);

    if (!::AttachConsole(ATTACH_PARENT_PROCESS)) {
        ::AllocConsole();
    }

    if (has_redirected_stderr) {
        return;
    }

    FILE *stream = nullptr;
    if (freopen_s(&stream, "CONOUT$", "w", stderr) != 0) {
        // 標準エラー出力が使えず診断メッセージが全て失われるため、デバッガから観測できる経路へ通知する
        ::OutputDebugStringA("hengband: failed to attach stderr\n");
    }
}

/*!
 * @brief (Windows固有)画面をHTMLファイルに保存する
 * @details
 * ファイル保存ダイアログを表示し、指定のファイルに画面内容を保存する。
 * @param hWnd ダイアログの親にするウインドウのハンドル
 */
void save_screen_as_html(HWND hWnd)
{
    std::vector<WCHAR> buf(MAIN_WIN_MAX_PATH + 1);
    OPENFILENAMEW ofnw{};
    ofnw.lStructSize = sizeof(ofnw);
    ofnw.hwndOwner = hWnd;
    ofnw.lpstrFilter = L"HTML Files (*.html)\0*.html\0";
    ofnw.nFilterIndex = 1;
    ofnw.lpstrFile = &buf[0];
    ofnw.nMaxFile = MAIN_WIN_MAX_PATH;
    ofnw.lpstrDefExt = L"html";
    ofnw.lpstrInitialDir = NULL;
    ofnw.lpstrTitle = _(L"HTMLでスクリーンダンプを保存", L"Save screen dump as HTML.");
    ofnw.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofnw)) {
        exe_cmd_save_screen_html(to_multibyte(&buf[0]).c_str(), false);
    }
}

/*!
 * @brief 対象ファイルを選択した状態でエクスプローラーを開く
 * @param path 対象ファイルのパス
 */
void open_dir_in_explorer(const std::filesystem::path &path)
{
    std::wstringstream ss;
    ss << L"/select," << path.wstring();
    ShellExecuteW(NULL, NULL, L"explorer.exe", ss.str().data(), NULL, SW_SHOWNORMAL);
}

/*!
 * @brief GetOpenFileNameW APIのラッパー
 * @param ofn GetOpenFileNameWに指定するOPENFILENAMEW構造体へのポインタ。
 * lpstrFile、nMaxFileメンバの設定は無視される（関数内で上書きするため）。
 * @param path_dir GetOpenFileNameWに指定する初期フォルダパス。
 * @param path_file 初期選択ファイルパス
 * @param max_name_size 選択ファイルパスの最大長
 * @return 選択されたファイルパス。選択をキャンセルした場合はtl::nullopt。
 */
tl::optional<std::filesystem::path> get_open_filename(OPENFILENAMEW *ofn, const std::filesystem::path &path_dir, const std::filesystem::path &path_file, DWORD max_name_size)
{
    std::vector<WCHAR> buf(max_name_size);
    const auto path_file_str = path_file.wstring();
    const auto path_dir_str = path_dir.wstring();

    if (path_file_str.length() < buf.size()) {
        std::copy(path_file_str.begin(), path_file_str.end(), buf.begin());
    }

    ofn->lpstrFile = buf.data();
    ofn->nMaxFile = buf.size();
    ofn->lpstrInitialDir = path_dir_str.empty() ? nullptr : path_dir_str.data();

    if (GetOpenFileNameW(ofn)) {
        return tl::make_optional<std::filesystem::path>(buf.data());
    }

    return tl::nullopt;
}
