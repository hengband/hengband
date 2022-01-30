/*!
 * @file main-win-utils.cpp
 * @brief Windows版固有実装(ユーティリティー)
 */

#include "main-win/main-win-utils.h"
#include "cmd-io/cmd-process-screen.h"
#include "locale/language-switcher.h"
#include "main-win/main-win-define.h"
#include "system/angband-version.h"

#include <string>

/*!
 * @brief (Windows固有)変愚蛮怒が起動済かどうかのチェック
 * @details
 * 特定の名前のミューテックスオブジェクトの所有権取得を試みる。
 * 取得できない場合は他に変愚蛮怒のプロセスが起動しているとみなす。
 * 取得したミューテックスのハンドルは明示的な解放は行わず、プロセス終了時にOSが解放する。
 * @return 起動済の変愚蛮怒プロセス有無
 */
bool is_already_running(void)
{
    wchar_t wtext[32];
    mbstowcs(wtext, VARIANT_NAME.data(), VARIANT_NAME.length());
    [[maybe_unused]] HANDLE hMutex = CreateMutexW(NULL, TRUE, wtext);
    return GetLastError() == ERROR_ALREADY_EXISTS;
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
void open_dir_in_explorer(char *filename)
{
    std::string str = "/select," + std::string(filename);
    ShellExecuteW(NULL, NULL, L"explorer.exe", to_wchar(str.c_str()).wc_str(), NULL, SW_SHOWNORMAL);
}

/*!
 * @brief GetOpenFileNameW APIのラッパー
 * @details
 * ワイド文字列版のAPIを使用するが、選択ファイルのパスをマルチバイト文字列で受け取る。
 * @param ofn GetOpenFileNameWに指定するOPENFILENAMEW構造体へのポインタ。
 * lpstrFile、nMaxFileメンバの設定は無視される（関数内で上書きするため）。
 * @param dirname GetOpenFileNameWに指定する初期フォルダパス。
 * NULL以外を指定した場合、ワイド文字列に変換しlpstrInitialDirに設定される。
 * @param filename 選択ファイルパス設定先バッファへのポインタ
 * @param max_name_size filenameのバッファサイズ
 * @retval true filenameに選択されたファイルのパスが設定されている。
 * @retval false ファイル選択がキャンセルされた。
 */
bool get_open_filename(OPENFILENAMEW *ofn, concptr dirname, char *filename, DWORD max_name_size)
{
    std::vector<WCHAR> buf(max_name_size);
    wcscpy(&buf[0], to_wchar(filename).wc_str());
    to_wchar wc_dir(dirname);

    // Overwrite struct data
    ofn->lpstrFile = &buf[0];
    ofn->nMaxFile = max_name_size - 1;
    ofn->lpstrInitialDir = wc_dir.wc_str();

    // call API
    if (GetOpenFileNameW(ofn)) {
        // to multibyte
        strncpy_s(filename, max_name_size, to_multibyte(&buf[0]).c_str(), _TRUNCATE);
        return true;
    }
    
    return false;
}
