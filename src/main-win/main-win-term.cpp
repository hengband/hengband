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

/*!
 * @brief Resize a window
 */
void term_data::resize_window()
{
    if (!this->w) {
        return;
    }

    SetWindowPos(this->w, 0, 0, 0, this->size_wid, this->size_hgt, SWP_NOMOVE | SWP_NOZORDER);
    if (this->size_hack) {
        return;
    }

    this->dispose_offscreen();
    term_activate(&this->t);
    term_redraw();
}

/*!
 * @brief (Windows版固有実装) / Adjust the "size" for a window
 */
void term_data::adjust_size()
{
    if (this->cols < 1) {
        this->cols = 1;
    }

    if (this->rows < 1) {
        this->rows = 1;
    }

    TERM_LEN wid = this->cols * this->tile_wid + this->size_ow1 + this->size_ow2;
    TERM_LEN hgt = this->rows * this->tile_hgt + this->size_oh1 + this->size_oh2;

    RECT rw, rc;
    if (this->w) {
        GetWindowRect(this->w, &rw);
        GetClientRect(this->w, &rc);

        this->size_wid = (rw.right - rw.left) - (rc.right - rc.left) + wid;
        this->size_hgt = (rw.bottom - rw.top) - (rc.bottom - rc.top) + hgt;

        this->pos_x = rw.left;
        this->pos_y = rw.top;
    } else {
        /* Tempolary calculation */
        rc.left = 0;
        rc.right = wid;
        rc.top = 0;
        rc.bottom = hgt;
        AdjustWindowRectEx(&rc, this->dwStyle, TRUE, this->dwExStyle);
        this->size_wid = rc.right - rc.left;
        this->size_hgt = rc.bottom - rc.top;
    }
}

/*!
 * @brief Force the use of a new font for a term_data.
 * This function may be called before the "window" is ready.
 * This function returns zero only if everything succeeds.
 * @note that the "font name" must be capitalized!!!
 */
void term_data::force_font()
{
    if (this->font_id) {
        DeleteObject(this->font_id);
    }

    this->font_id = CreateFontIndirectW(&(this->lf));
    int wid = this->lf.lfWidth;
    int hgt = this->lf.lfHeight;
    if (!this->font_id) {
        return;
    }

    if ((wid == 0) || (hgt == 0)) {
        auto hdcDesktop = GetDC(HWND_DESKTOP);
        auto hfOld = static_cast<HFONT>(SelectObject(hdcDesktop, this->font_id));
        TEXTMETRIC tm;
        GetTextMetrics(hdcDesktop, &tm);
        SelectObject(hdcDesktop, hfOld);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
        wid = tm.tmAveCharWidth;
        hgt = tm.tmHeight;
    }

    this->font_wid = wid;
    this->font_hgt = hgt;
    return;
}

/*!
 * @brief Allow the user to change the font for this window.
 */
void term_data::change_font()
{
    CHOOSEFONTW cf;
    memset(&cf, 0, sizeof(cf));
    cf.lStructSize = sizeof(cf);
    cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
    cf.lpLogFont = &(this->lf);

    if (!ChooseFontW(&cf))
        return;

    this->force_font();
    this->tile_wid = this->font_wid;
    this->tile_hgt = this->font_hgt;
    this->adjust_size();
    this->resize_window();
}
