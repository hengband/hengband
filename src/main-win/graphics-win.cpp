/*!
 * @file graphics-win.cpp
 * @brief Windows版固有実装(タイル、イメージファイルの読み込み)
 */

#include "main-win/graphics-win.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-utils.h"
#include "system/system-variables.h"
#include "util/angband-files.h"

#pragma warning(push)
#pragma warning(disable : 4458)
#include <gdiplus.h>
#pragma warning(pop)

// Flag set once "GDI+" has been initialized
bool gdi_plus_started = false;
// a token for "GDI+"
ULONG_PTR gdiplusToken;

// interface object
Graphics graphic{};

concptr ANGBAND_DIR_XTRA_GRAF;

/*!
 * 現在使用中のタイルID(0ならば未使用)
 */
static graphics_mode current_graphics_mode = graphics_mode::GRAPHICS_NONE;

/*
 * The global tile
 */
static tile_info infGraph;

/*!
 * @brief Initialize GDI+
 */
static inline void init_gdi_plus()
{
    if (!gdi_plus_started) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        gdi_plus_started = true;
    }
}

/*!
 * @brief Cleans up resources used by GDI+
 */
static void finalize_gdi_plus()
{
    if (gdi_plus_started) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        gdi_plus_started = false;
    }
}

HBITMAP read_graphic(char *filename)
{
    HBITMAP result = NULL;
    init_gdi_plus();

    Gdiplus::Bitmap bitmap(to_wchar(filename).wc_str());

    COLORREF bgcolor = RGB(0x00, 0x00, 0x00);
    bitmap.GetHBITMAP(bgcolor, &result);

    return result;
}

namespace Impl {
graphics_mode change_graphics(graphics_mode arg)
{
    if (current_graphics_mode == arg) {
        return current_graphics_mode;
    }

    char buf[MAIN_WIN_MAX_PATH];
    BYTE wid, hgt, twid, thgt, ox, oy;
    std::string name;
    std::string name_mask("");

    infGraph.delete_bitmap();

    if (arg == graphics_mode::GRAPHICS_ORIGINAL) {
        wid = 8;
        hgt = 8;
        twid = 8;
        thgt = 8;
        ox = 0;
        oy = 0;
        name = "8X8.BMP";
        ANGBAND_GRAF = "old";
    } else if (arg == graphics_mode::GRAPHICS_ADAM_BOLT) {
        wid = 16;
        hgt = 16;
        twid = 16;
        thgt = 16;
        ox = 0;
        oy = 0;
        name = "16X16.BMP";
        name_mask = "mask.bmp";

        ANGBAND_GRAF = "new";
    } else if (arg == graphics_mode::GRAPHICS_HENGBAND) {
        wid = 32;
        hgt = 32;
        twid = 32;
        thgt = 32;
        ox = 0;
        oy = 0;
        name = "32X32.BMP";
        name_mask = "mask32.bmp";

        ANGBAND_GRAF = "ne2";
    } else {
        ANGBAND_GRAF = "ascii";
        current_graphics_mode = graphics_mode::GRAPHICS_NONE;
        return current_graphics_mode;
    }

    path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA_GRAF, name);
    infGraph.hBitmap = read_graphic(buf);
    if (!infGraph.hBitmap) {
        plog_fmt(_("ビットマップ '%s' を読み込めません。", "Cannot read bitmap file '%s'"), name.data());
        ANGBAND_GRAF = "ascii";
        current_graphics_mode = graphics_mode::GRAPHICS_NONE;
        return current_graphics_mode;
    }

    infGraph.CellWidth = wid;
    infGraph.CellHeight = hgt;
    infGraph.TileWidth = twid;
    infGraph.TileHeight = thgt;
    infGraph.OffsetX = ox;
    infGraph.OffsetY = oy;

    if (name_mask.empty()) {
        path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA_GRAF, name_mask);
        infGraph.hBitmapMask = read_graphic(buf);
        if (!infGraph.hBitmapMask) {
            plog_fmt(_("ビットマップ '%s' を読み込めません。", "Cannot read bitmap file '%s'"), name_mask.data());
            ANGBAND_GRAF = "ascii";
            current_graphics_mode = graphics_mode::GRAPHICS_NONE;
            return current_graphics_mode;
        }
    }

    current_graphics_mode = arg;
    return arg;
}
}

graphics_mode Graphics::get_mode(void)
{
    return current_graphics_mode;
}

graphics_mode Graphics::change_graphics(graphics_mode arg)
{
    return Impl::change_graphics(arg);
}

const tile_info &Graphics::get_tile_info(void)
{
    return infGraph;
}

void Graphics::init(void)
{
    init_gdi_plus();
}

void Graphics::finalize()
{
    infGraph.delete_bitmap();
    finalize_gdi_plus();
}
