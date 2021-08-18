/*!
 * @file main-win-term.cpp
 * @brief Windows版固有実装(ターミナル)
 */

#include "main-win/main-win-term.h"

term_data data[MAX_TERM_DATA];

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

/*!
 * @brief オフスクリーンHDC取得
 * @return HDC
 */
HDC term_data::get_hdc()
{
    return graphics.get_hdc(this->w);
}

/*!
 * @brief WM_PAINTでの描画が必要になる領域を設定する.
 * @param lpRect 更新領域。NULLの場合はクライアント領域全体の描画が必要。
 */
void term_data::refresh(const RECT *lpRect)
{
    InvalidateRect(this->w, lpRect, FALSE);
}

/*!
 * @brief オフスクリーン内容を表示画面に描画する
 * @param rect 描画領域
 * @retval true 描画した
 * @retval false 描画なし（オフスクリーンが存在しない等）
 */
bool term_data::render(const RECT &rect)
{
    return graphics.render(rect);
}

/*!
 * @brief オフスクリーンを破棄する
 * @details
 * リサイズ時にはオフスクリーンを破棄して
 * 呼び出し元で画面の再描画を行う必要がある。
 */
void term_data::dispose_offscreen()
{
    graphics.dispose_offscreen();
};

bool term_data::is_main_term()
{
    return (this == &data[0]);
}
