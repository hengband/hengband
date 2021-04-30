/*!
 * @file main-win-utils.cpp
 * @brief Windows版固有実装(ユーティリティー)
 */

#include "main-win/main-win-utils.h"
#include "cmd-io/cmd-process-screen.h"
#include "locale/language-switcher.h"
#include "main-win/main-win-define.h"
#include "system/angband-version.h"

/*!
 * @brief (Windows固有)変愚蛮怒が起動済かどうかのチェック
 * @details
 * 特定の名前のミューテックスオブジェクトの所有権取得を試みる。
 * 取得できない場合は他に変愚蛮怒のプロセスが起動しているとみなす。
 * 取得したミューテックスのハンドルは明示的な解放は行わず、プロセス終了時にOSが解放する。
 * @retval true 他に変愚蛮怒のプロセスが起動している
 * @retval false 他に変愚蛮怒のプロセスは起動していない
 */
bool is_already_running(void)
{
    [[maybe_unused]] HANDLE hMutex = CreateMutex(NULL, TRUE, VERSION_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }

    return false;
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
    OPENFILENAMEW ofnw;

    memset(&ofnw, 0, sizeof(ofnw));
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
        do_cmd_save_screen_html_aux(to_multibyte(&buf[0]).c_str(), 0);
    }
}

/*!
 * @brief 対象ファイルを選択した状態でエクスプローラーを開く
 * @param filename 対象ファイル
 */
void open_dir_in_explorer(char *filename) {
    std::string str = "/select," + std::string(filename);
    ShellExecuteW(NULL, NULL, L"explorer.exe", to_wchar(str.c_str()).wc_str(), NULL, SW_SHOWNORMAL);
}
