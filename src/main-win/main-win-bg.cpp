/*!
 * @file main-win-bg.cpp
 * @brief Windows版固有実装(壁紙)
 */

#include "main-win/main-win-bg.h"
#include "locale/language-switcher.h"
#include "system/h-define.h"
#include "term/z-form.h"

#include <gdiplus.h>

static HBITMAP hBG = NULL;
char bg_bitmap_file[MAIN_WIN_MAX_PATH] = ""; //!< 現在の背景ビットマップファイル名。

static bool gdi_plus_started = false;
static ULONG_PTR gdiplusToken;

static void init_gdi_plus()
{
    if (!gdi_plus_started) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        gdi_plus_started = true;
    }
}

void finalize_bg()
{
    delete_bg();
    if (gdi_plus_started) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }
}

void load_bg_prefs(void)
{
    init_gdi_plus();

    // TODO 背景の設定読込
}

void delete_bg(void)
{
    if (hBG != NULL) {
        DeleteObject(hBG);
        hBG = NULL;
    }
}

static BOOL init_bg_impl(void)
{
    delete_bg();

    wchar_t wc[MAIN_WIN_MAX_PATH] = L"";
    mbstowcs(wc, bg_bitmap_file, MAIN_WIN_MAX_PATH - 1);
    Gdiplus::Bitmap bitmap(wc);

    COLORREF bgcolor = RGB(0x00, 0x00, 0x00);
    Gdiplus::Status status = bitmap.GetHBITMAP(bgcolor, &hBG);
    if ((status != Gdiplus::Ok) || !hBG) {
        return FALSE;
    }

    return TRUE;
}

BOOL init_bg(void)
{
    BOOL result = init_bg_impl();
    if (!result) {
        plog_fmt(_("壁紙用ファイル '%s' を読み込めません。", "Can't load the image file '%s'."), bg_bitmap_file);
    }

    return result;
}

void draw_bg(HDC hdc, RECT *r)
{
    if (!hBG)
        return;

    int x = r->left, y = r->top;
    int nx = x;
    int ny = y;
    BITMAP bm;
    GetObject(hBG, sizeof(bm), &bm);
    int swid = bm.bmWidth;
    int shgt = bm.bmHeight;

    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP hOld = static_cast<HBITMAP>(SelectObject(hdcSrc, hBG));

    do {
        int sx = nx % swid;
        int cwid = MIN(swid - sx, r->right - nx);
        do {
            int sy = ny % shgt;
            int chgt = MIN(shgt - sy, r->bottom - ny);
            BitBlt(hdc, nx, ny, cwid, chgt, hdcSrc, sx, sy, SRCCOPY);
            ny += chgt;
        } while (ny < r->bottom);

        ny = y;
        nx += cwid;
    } while (nx < r->right);

    SelectObject(hdcSrc, hOld);
    DeleteDC(hdcSrc);
}
