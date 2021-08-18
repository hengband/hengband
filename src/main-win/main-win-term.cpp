/*!
 * @file main-win-term.cpp
 * @brief Windows版固有実装(ターミナル)
 */

#include "main-win/main-win-term.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "core/window-redrawer.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "main-win/main-win-music.h"
#include "main-win/main-win-sound.h"
#include "main-win/main-win-utils.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"

LPCWSTR AppName = L"ANGBAND";
HINSTANCE hInstance;
bg_mode current_bg_mode = bg_mode::BG_NONE;
COLORREF win_clr[256];
POINT normsize;
term_data data[MAX_TERM_DATA];
HBRUSH hbrYellow;

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

/*!
 * @brief Allow the user to lock this window.
 */
void term_data::set_window_position(HWND hWnd)
{
    SetWindowPos(this->w, hWnd, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

/*!
 * @brief Hack -- redraw a term_data
 */
void term_data::redraw_data()
{
    term_activate(&this->t);
    term_redraw();
    term_activate(term_screen);
}

/*!
 * @brief ターミナルのサイズ更新
 * @details 行数、列数の変更に対応する。
 * @param td term_dataのポインタ
 * @param resize_window trueの場合に再計算されたウインドウサイズにリサイズする
 */
void term_data::rebuild(bool resize_window)
{
    term_type *old = Term;
    this->size_hack = true;
    term_activate(&this->t);
    this->adjust_size();
    if (resize_window) {
        this->resize_window();
    }
    this->dispose_offscreen();
    term_resize(this->cols, this->rows);
    this->size_hack = false;
    term_activate(old);
}

void term_data::fit_size_to_window(bool recalc_window_size)
{
    RECT rc;
    ::GetClientRect(this->w, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    TERM_LEN tmp_cols = (width - this->size_ow1 - this->size_ow2) / this->tile_wid;
    TERM_LEN tmp_rows = (height - this->size_oh1 - this->size_oh2) / this->tile_hgt;
    if ((this->cols == tmp_cols) && (this->rows == tmp_rows)) {
        return;
    }

    this->cols = tmp_cols;
    this->rows = tmp_rows;
    if (this->is_main_term() && !IsZoomed(this->w) && !IsIconic(this->w)) {
        normsize.x = this->cols;
        normsize.y = this->rows;
    }

    this->rebuild(recalc_window_size);

    if (this->is_main_term()) {
        return;
    }

    p_ptr->window_flags = PW_ALL;
    handle_stuff(p_ptr);
}

/*!
 * @brief Create and initialize a "term_data" given a title
 */
void term_data::link_data()
{
    auto *link = &this->t;
    term_init(link, this->cols, this->rows, FILE_READ_BUFF_SIZE);
    link->soft_cursor = true;
    link->higher_pict = true;
    link->attr_blank = TERM_WHITE;
    link->char_blank = ' ';
    link->user_hook = this->term_user_win;
    link->xtra_hook = this->term_xtra_win;
    link->curs_hook = this->term_curs_win;
    link->bigcurs_hook = this->term_bigcurs_win;
    link->wipe_hook = this->term_wipe_win;
    link->text_hook = this->term_text_win;
    link->pict_hook = this->term_pict_win;
    link->data = (vptr)(this);
}

/*!
 * @brief Windows版ユーザ設定項目実装部(実装必須) /Interact with the User
 */
errr term_data::term_user_win(int n)
{
    (void)n;
    return 0;
}

/*!
 * @brief Low level graphics (Assumes valid input).
 * @details
 * Draw a "big cursor" at (x,y), using a "yellow box".
 */
errr term_data::term_bigcurs_win(int x, int y)
{
    term_data *td = (term_data *)(Term->data);
    int tile_wid, tile_hgt;
    tile_wid = td->tile_wid;
    tile_hgt = td->tile_hgt;

    RECT rc;
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + 2 * tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = td->get_hdc();
    FrameRect(hdc, &rc, hbrYellow);
    td->refresh(&rc);
    return 0;
}

/*!
 * @brief Low level graphics (Assumes valid input).
 * @details
 * Erase a "block" of "n" characters starting at (x,y).
 */
errr term_data::term_wipe_win(int x, int y, int n)
{
    term_data *td = (term_data *)(Term->data);
    RECT rc;
    rc.left = x * td->tile_wid + td->size_ow1;
    rc.right = rc.left + n * td->tile_wid;
    rc.top = y * td->tile_hgt + td->size_oh1;
    rc.bottom = rc.top + td->tile_hgt;

    HDC hdc = td->get_hdc();
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    if (current_bg_mode != bg_mode::BG_NONE)
        draw_bg(hdc, &rc);
    else
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    td->refresh(&rc);
    return 0;
}

/*!
 * @brief Low level graphics.  Assumes valid input.
 * @details
 * Draw several ("n") chars, with an attr, at a given location.
 *
 * All "graphic" data is handled by "term_pict_win()", below.
 *
 * One would think there is a more efficient method for telling a window
 * what color it should be using to draw with, but perhaps simply changing
 * it every time is not too inefficient.
 */
errr term_data::term_text_win(int x, int y, int n, TERM_COLOR a, concptr s)
{
    term_data *td = (term_data *)(Term->data);
    static HBITMAP WALL;
    static HBRUSH myBrush, oldBrush;
    static HPEN oldPen;
    static bool init_done = false;

    if (!init_done) {
        WALL = LoadBitmapW(hInstance, AppName);
        myBrush = CreatePatternBrush(WALL);
        init_done = true;
    }

    RECT rc{ static_cast<LONG>(x * td->tile_wid + td->size_ow1), static_cast<LONG>(y * td->tile_hgt + td->size_oh1),
        static_cast<LONG>(rc.left + n * td->tile_wid), static_cast<LONG>(rc.top + td->tile_hgt) };
    RECT rc_start = rc;

    HDC hdc = td->get_hdc();
    SetBkColor(hdc, RGB(0, 0, 0));
    SetTextColor(hdc, win_clr[a]);

    SelectObject(hdc, td->font_id);
    if (current_bg_mode != bg_mode::BG_NONE)
        SetBkMode(hdc, TRANSPARENT);

    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    if (current_bg_mode != bg_mode::BG_NONE)
        draw_bg(hdc, &rc);

    rc.left += ((td->tile_wid - td->font_wid) / 2);
    rc.right = rc.left + td->font_wid;
    rc.top += ((td->tile_hgt - td->font_hgt) / 2);
    rc.bottom = rc.top + td->font_hgt;

    for (int i = 0; i < n; i++) {
#ifdef JP
        if (use_bigtile && *(s + i) == "■"[0] && *(s + i + 1) == "■"[1]) {
            rc.right += td->font_wid;
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
            Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            rc.right -= td->font_wid;
            i++;
            rc.left += 2 * td->tile_wid;
            rc.right += 2 * td->tile_wid;
        } else if (iskanji(*(s + i))) /* 2バイト文字 */
        {
            char tmp[] = { *(s + i), *(s + i + 1), '\0' };
            to_wchar wc(tmp);
            const auto *buf = wc.wc_str();
            rc.right += td->font_wid;
            if (buf == NULL)
                ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 2, NULL);
            else
                ExtTextOutW(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, buf, wcslen(buf), NULL);
            rc.right -= td->font_wid;
            i++;
            rc.left += 2 * td->tile_wid;
            rc.right += 2 * td->tile_wid;
        } else if (*(s + i) == 127) {
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
            Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        } else {
            ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 1, NULL);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        }
#else
        if (*(s + i) == 127) {
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
            Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        } else {
            ExtTextOutA(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 1, NULL);
            rc.left += td->tile_wid;
            rc.right += td->tile_wid;
        }
#endif
    }

    rc.left = rc_start.left;
    rc.top = rc_start.top;
    td->refresh(&rc);
    return 0;
}

/*!
 * @brief Low level graphics.  Assumes valid input.
 * @details
 * Draw an array of "special" attr/char pairs at the given location.
 *
 * We use the "term_pict_win()" function for "graphic" data, which are
 * encoded by setting the "high-bits" of both the "attr" and the "char"
 * data.  We use the "attr" to represent the "row" of the main bitmap,
 * and the "char" to represent the "col" of the main bitmap.  The use
 * of this function is induced by the "higher_pict" flag.
 *
 * If "graphics" is not available, we simply "wipe" the given grids.
 */
errr term_data::term_pict_win(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, concptr cp, const TERM_COLOR *tap, concptr tcp)
{
    term_data *td = (term_data *)(Term->data);
    int i;
    HDC hdcMask = NULL;
    if (!use_graphics) {
        return (term_wipe_win(x, y, n));
    }

    const tile_info &infGraph = graphic.get_tile_info();
    const bool has_mask = (infGraph.hBitmapMask != NULL);
    TERM_LEN w1 = infGraph.CellWidth;
    TERM_LEN h1 = infGraph.CellHeight;
    TERM_LEN tw1 = infGraph.TileWidth;
    TERM_LEN th1 = infGraph.TileHeight;
    TERM_LEN w2, h2, tw2 = 0;
    w2 = td->tile_wid;
    h2 = td->tile_hgt;
    tw2 = w2;
    if (use_bigtile)
        tw2 *= 2;

    TERM_LEN x2 = x * w2 + td->size_ow1 + infGraph.OffsetX;
    TERM_LEN y2 = y * h2 + td->size_oh1 + infGraph.OffsetY;
    HDC hdc = td->get_hdc();
    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP hbmSrcOld = static_cast<HBITMAP>(SelectObject(hdcSrc, infGraph.hBitmap));

    if (has_mask) {
        hdcMask = CreateCompatibleDC(hdc);
        SelectObject(hdcMask, infGraph.hBitmapMask);
    }

    for (i = 0; i < n; i++, x2 += w2) {
        TERM_COLOR a = ap[i];
        char c = cp[i];
        int row = (a & 0x7F);
        int col = (c & 0x7F);
        TERM_LEN x1 = col * w1;
        TERM_LEN y1 = row * h1;

        if (has_mask) {
            TERM_LEN x3 = (tcp[i] & 0x7F) * w1;
            TERM_LEN y3 = (tap[i] & 0x7F) * h1;
            tw2 = tw2 * w1 / tw1;
            h2 = h2 * h1 / th1;
            if ((tw1 == tw2) && (th1 == h2)) {
                BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x3, y3, SRCCOPY);
                BitBlt(hdc, x2, y2, tw2, h2, hdcMask, x1, y1, SRCAND);
                BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, SRCPAINT);
                continue;
            }

            SetStretchBltMode(hdc, COLORONCOLOR);
            StretchBlt(hdc, x2, y2, tw2, h2, hdcMask, x3, y3, w1, h1, SRCAND);
            StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x3, y3, w1, h1, SRCPAINT);
            if ((x1 != x3) || (y1 != y3)) {
                StretchBlt(hdc, x2, y2, tw2, h2, hdcMask, x1, y1, w1, h1, SRCAND);
                StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, w1, h1, SRCPAINT);
            }

            continue;
        }

        if ((w1 == tw2) && (h1 == h2)) {
            BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, SRCCOPY);
            continue;
        }

        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, w1, h1, SRCCOPY);
    }

    SelectObject(hdcSrc, hbmSrcOld);
    DeleteDC(hdcSrc);
    if (has_mask) {
        SelectObject(hdcMask, hbmSrcOld);
        DeleteDC(hdcMask);
    }

    td->refresh();
    return 0;
}

/*!
 * @brief カラーパレットの変更？
 */
void term_data::refresh_color_table()
{
    for (int i = 0; i < 256; i++) {
        byte rv = angband_color_table[i][1];
        byte gv = angband_color_table[i][2];
        byte bv = angband_color_table[i][3];
        win_clr[i] = PALETTERGB(rv, gv, bv);
    }
}

/*!
 * @brief グラフィクスのモード変更
 */
void term_data::change_graphics_mode(graphics_mode mode)
{
    graphics_mode ret = graphic.change_graphics(mode);
    if (ret != mode) {
        plog(_("グラフィクスを初期化できません!", "Cannot initialize graphics!"));
    }
    arg_graphics = static_cast<byte>(ret);
    use_graphics = (arg_graphics > 0);
}

/*!
 * @brief Process all pending events
 */
errr term_data::term_xtra_win_flush()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

/*!
 * @brief Hack -- clear the screen
 * @details
 * Make this more efficient
 */
errr term_data::term_xtra_win_clear(void)
{
    term_data *td = (term_data *)(Term->data);

    RECT rc;
    GetClientRect(td->w, &rc);

    HDC hdc = td->get_hdc();
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    if (current_bg_mode != bg_mode::BG_NONE) {
        rc.left = 0;
        rc.top = 0;
        draw_bg(hdc, &rc);
    }

    td->refresh();
    return 0;
}

/*!
 * @brief Hack -- make a noise
 */
errr term_data::term_xtra_win_noise(void)
{
    MessageBeep(MB_ICONASTERISK);
    return 0;
}

/*!
 * @brief Hack -- make a sound
 */
errr term_data::term_xtra_win_sound(int v)
{
    if (!use_sound)
        return 1;
    return play_sound(v);
}

/*!
 * @brief Hack -- play a music
 */
errr term_data::term_xtra_win_music(int n, int v)
{
    if (!use_music) {
        return 1;
    }

    return main_win_music::play_music(n, v);
}

/*!
 * @brief Hack -- play a music matches a situation
 */
errr term_data::term_xtra_win_scene(int v)
{
    // TODO 場面に合った壁紙変更対応
    if (!use_music) {
        return 1;
    }

    return main_win_music::play_music_scene(v);
}

/*!
 * @brief Delay for "x" milliseconds
 */
int term_data::term_xtra_win_delay(int v)
{
    Sleep(v);
    return 0;
}

/*!
 * @brief Do a "special thing"
 * @todo z-termに影響があるのでplayer_typeの追加は保留
 */
errr term_data::term_xtra_win(int n, int v)
{
    switch (n) {
    case TERM_XTRA_NOISE: {
        return (term_xtra_win_noise());
    }
    case TERM_XTRA_FRESH: {
        term_data *td = (term_data *)(Term->data);
        if (td->w)
            UpdateWindow(td->w);
        return 0;
    }
    case TERM_XTRA_MUSIC_BASIC:
    case TERM_XTRA_MUSIC_DUNGEON:
    case TERM_XTRA_MUSIC_QUEST:
    case TERM_XTRA_MUSIC_TOWN:
    case TERM_XTRA_MUSIC_MONSTER: {
        return term_xtra_win_music(n, v);
    }
    case TERM_XTRA_MUSIC_MUTE: {
        return main_win_music::stop_music();
    }
    case TERM_XTRA_SCENE: {
        return term_xtra_win_scene(v);
    }
    case TERM_XTRA_SOUND: {
        return (term_xtra_win_sound(v));
    }
    case TERM_XTRA_BORED: {
        return (term_xtra_win_event(0));
    }
    case TERM_XTRA_EVENT: {
        return (term_xtra_win_event(v));
    }
    case TERM_XTRA_FLUSH: {
        return (term_xtra_win_flush());
    }
    case TERM_XTRA_CLEAR: {
        return (term_xtra_win_clear());
    }
    case TERM_XTRA_REACT: {
        return (term_xtra_win_react(p_ptr));
    }
    case TERM_XTRA_DELAY: {
        return (term_xtra_win_delay(v));
    }
    }

    return 1;
}

/*!
 * @brief Low level graphics (Assumes valid input).
 * @details
 * Draw a "cursor" at (x,y), using a "yellow box".
 */
errr term_data::term_curs_win(int x, int y)
{
    term_data *td = (term_data *)(Term->data);
    int tile_wid, tile_hgt;
    tile_wid = td->tile_wid;
    tile_hgt = td->tile_hgt;

    RECT rc;
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = td->get_hdc();
    FrameRect(hdc, &rc, hbrYellow);
    td->refresh(&rc);
    return 0;
}

/*!
 * @brief React to global changes
 */
errr term_data::term_xtra_win_react(player_type *player_ptr)
{
    refresh_color_table();

    const byte current_mode = static_cast<byte>(graphic.get_mode());
    if (current_mode != arg_graphics) {
        change_graphics_mode(static_cast<graphics_mode>(arg_graphics));
        reset_visuals(player_ptr);
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        term_data *td = &data[i];
        if ((td->cols != td->t.wid) || (td->rows != td->t.hgt)) {
            td->rebuild();
        }
    }

    return 0;
}

/*!
 * @brief Process at least one event
 */
errr term_data::term_xtra_win_event(int v)
{
    MSG msg;
    if (v) {
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
