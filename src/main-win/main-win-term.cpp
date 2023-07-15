/*!
 * @file main-win-term.cpp
 * @brief Windows版固有実装(ターミナル)
 */

#include "main-win/main-win-term.h"

/*!
 * @brief オフスクリーンのデバイスコンテキストを取得する
 * @param hWnd ウインドウハンドル
 * @return デバイスコンテキスト
 */
HDC double_buffering::get_hdc(HWND hWnd)
{
    if (!this->offscreen) {
        RECT rc;
        this->display = ::GetDC(hWnd);
        ::GetClientRect(hWnd, &rc);
        this->bitmap = ::CreateCompatibleBitmap(this->display, rc.right, rc.bottom);
        this->offscreen = ::CreateCompatibleDC(this->display);
        ::SelectObject(this->offscreen, this->bitmap);
    }
    return this->offscreen;
}

/*!
 * @brief オフスクリーン内容を表示画面に描画する
 * @param rect 描画範囲
 * @retval true 描画した
 * @retval false 描画なし。オフスクリーンが存在しない。
 */
bool double_buffering::render(const RECT &rect)
{
    if (this->offscreen) {
        ::BitBlt(this->display, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, this->offscreen, rect.left, rect.top, SRCCOPY);
        return true;
    } else {
        return false;
    }
}

/*!
 * @brief オフスクリーンを破棄する。
 * @details
 * リサイズ時にはオフスクリーンを破棄して
 * 呼び出し元で画面の再描画を行う必要がある。
 */
void double_buffering::dispose_offscreen()
{
    ::DeleteDC(this->offscreen);
    this->offscreen = NULL;
    ::DeleteObject(this->bitmap);
    this->bitmap = NULL;
    ::DeleteDC(this->display);
    this->display = NULL;
}
