/*!
 * @file main-win-bg.cpp
 * @brief Windows版固有実装(壁紙)
 */

#include "main-win/main-win-bg.h"
#include "main-win/graphics-win.h"
#include "system/h-define.h"
#include <algorithm>

HBITMAP hBG = NULL;

void finalize_bg()
{
    delete_bg();
}

void load_bg_prefs(void)
{
    // TODO 背景の設定読込
}

void delete_bg(void)
{
    if (hBG != NULL) {
        DeleteObject(hBG);
        hBG = NULL;
    }
}

bool load_bg(char* filename)
{
    delete_bg();
    hBG = read_graphic(filename);

    return (hBG != NULL);
}

void draw_bg(HDC hdc, RECT *r)
{
    if (!hBG)
        return;

    int x = r->left, y = r->top;
    int nx = x;
    int ny = y;
    BITMAP bm{};
    GetObject(hBG, sizeof(bm), &bm);
    int swid = bm.bmWidth;
    int shgt = bm.bmHeight;

    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP hOld = static_cast<HBITMAP>(SelectObject(hdcSrc, hBG));

    do {
        int sx = nx % swid;
        int cwid = std::min<int>(swid - sx, r->right - nx);
        do {
            int sy = ny % shgt;
            int chgt = std::min<int>(shgt - sy, r->bottom - ny);
            BitBlt(hdc, nx, ny, cwid, chgt, hdcSrc, sx, sy, SRCCOPY);
            ny += chgt;
        } while (ny < r->bottom);

        ny = y;
        nx += cwid;
    } while (nx < r->right);

    SelectObject(hdcSrc, hOld);
    DeleteDC(hdcSrc);
}
