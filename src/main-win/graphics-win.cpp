/*!
 * @file graphics-win.cpp
 * @brief Windows版固有実装(イメージファイルの読み込み)
 */

#include "main-win/graphics-win.h"
#include "main-win/main-win-define.h"

#pragma warning(push)
#pragma warning(disable : 4458)
#include <gdiplus.h>
#pragma warning(pop)

// Flag set once "GDI+" has been initialized
bool gdi_plus_started = false;
// a token for "GDI+"
ULONG_PTR gdiplusToken;

/*!
 * @brief Cleans up resources used by graphics
 */
static inline void init_graphics()
{
    if (!gdi_plus_started) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        gdi_plus_started = true;
    }
}

void finalize_graphics()
{
    if (gdi_plus_started) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
    }
}

HBITMAP read_graphic(char *filename)
{
    HBITMAP result = NULL;
    init_graphics();

    wchar_t wc[MAIN_WIN_MAX_PATH] = L"";
    ::mbstowcs(wc, filename, MAIN_WIN_MAX_PATH - 1);
    Gdiplus::Bitmap bitmap(wc);

    COLORREF bgcolor = RGB(0x00, 0x00, 0x00);
    bitmap.GetHBITMAP(bgcolor, &result);

    return result;
}
