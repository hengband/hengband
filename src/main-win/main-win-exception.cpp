#include "main-win/main-win-exception.h"
#include "main-win/main-win-utils.h"
#include "net/report-error.h"
#include <sstream>

/*!
 * @brief 予期しない例外を処理する
 *
 * 予期しない例外が発生した場合、確認を取り例外のエラー情報を開発チームに送信する。
 * その後、バグ報告ページを開くかどうか尋ね、開く場合はWebブラウザで開く。
 *
 * @param e 例外オブジェクト
 */
void handle_unexpected_exception(const std::exception &e)
{
    constexpr auto caption = _(L"予期しないエラー！", L"Unexpected error!");

#if !defined(DISABLE_NET)
    std::wstringstream report_confirm_msg_ss;
    report_confirm_msg_ss
        << to_wchar(e.what()).wc_str() << L"\n\n"
        << _(L"開発チームにエラー情報を送信してよろしいですか？\n", L"Are you sure you want to send the error information to the development team?\n")
        << _(L"※送信されるのはゲーム内の情報のみであり、個人情報が送信されることはありません。\n",
               L"Only in-game information will be sent. No personal information will be sent.\n");

    if (auto choice = MessageBoxW(NULL, report_confirm_msg_ss.str().data(), caption, MB_ICONEXCLAMATION | MB_YESNO | MB_ICONSTOP);
        choice == IDYES) {
        report_error(e.what());
    }
#endif

    std::wstringstream issue_page_open_msg_ss;
    issue_page_open_msg_ss
        << _(L"エラー発生の詳しい状況を報告してくださると助かります。\n",
               L"It would be helpful if you could report the detailed circumstances of the error.\n")
        << _(L"バグ報告ページを開きますか？\n", L"Open bug report page?\n");

    if (auto choice = MessageBoxW(NULL, issue_page_open_msg_ss.str().data(), caption, MB_ICONEXCLAMATION | MB_YESNO | MB_ICONSTOP);
        choice == IDYES) {
        constexpr auto url = "https://github.com/hengband/hengband/issues/new/choose";
        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    }
};
