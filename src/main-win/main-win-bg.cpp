/*!
 * @file main-win-bg.cpp
 * @brief Windows版固有実装(壁紙)
 */

#include "locale/language-switcher.h"
#include "main-win/main-win-bg.h"
#include "system/h-define.h"
#include "term/z-form.h"

static HBITMAP hBG = NULL;
char bg_bitmap_file[MAIN_WIN_MAX_PATH] = ""; //!< 現在の背景ビットマップファイル名。

void delete_bg(void)
{
    if (hBG != NULL) {
        DeleteObject(hBG);
        hBG = NULL;
    }
}

static BOOL init_bg_impl(void)
{
    char *bmfile = bg_bitmap_file;
    delete_bg();

    hBG = static_cast<HBITMAP>(LoadImage(NULL, bmfile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
    if (!hBG) {
        return FALSE;
    }

    return TRUE;
}

BOOL init_bg(void)
{
    BOOL result = init_bg_impl();
    if (!result) {
        plog_fmt(_("壁紙用ビットマップ '%s' を読み込めません。", "Can't load the bitmap file '%s'."), bg_bitmap_file);
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
