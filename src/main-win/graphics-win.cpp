/*!
 * @file graphics-win.cpp
 * @brief Windows版固有実装(タイル、イメージファイルの読み込み)
 */

#include "main-win/graphics-win.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-utils.h"
#include "system/system-variables.h"
#include "util/angband-files.h"
#include "view/actor-readability.h"
#include "view/original-item-catalog.h"
#include "view/original-monster-catalog.h"
#include "view/original-player-tiles.h"
#include "view/original-tiles.h"
#include <cstring>
#include <fmt/format.h>
#include <gdiplus.h>

// Flag set once "GDI+" has been initialized
bool gdi_plus_started = false;
// a token for "GDI+"
ULONG_PTR gdiplusToken;

// interface object
Graphics graphic{};

std::filesystem::path ANGBAND_DIR_XTRA_GRAF;

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

HBITMAP read_graphic(const std::filesystem::path &path)
{
    init_gdi_plus();
    Gdiplus::Bitmap bitmap(to_wchar(path.string()).wc_str());
    COLORREF bgcolor = RGB(0x00, 0x00, 0x00);
    HBITMAP result = NULL;
    bitmap.GetHBITMAP(bgcolor, &result);
    return result;
}

// 画像読込時に一度だけ補正し、描画では同じキャッシュ画像とタイル番号を使う。
// 元ファイルと地形・アイテムの画素は変更しない。
static HBITMAP readable_actor_bitmap(HBITMAP original)
{
    BITMAP bitmap{};
    if (GetObject(original, static_cast<int>(sizeof(bitmap)), &bitmap) != static_cast<int>(sizeof(bitmap)) || bitmap.bmWidth != 512 || bitmap.bmHeight <= 0) {
        return nullptr;
    }
    const auto width = static_cast<int>(bitmap.bmWidth);
    const auto height = static_cast<int>(bitmap.bmHeight);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    const auto dc = GetDC(nullptr);
    if (!dc) {
        return nullptr;
    }
    std::vector<uint32_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height));
    if (GetDIBits(dc, original, 0, static_cast<UINT>(height), pixels.data(), &info, DIB_RGB_COLORS) != height ||
        !original_tiles::make_actor_region_readable(pixels, width, height, original_tiles::MONSTER_BASE_ROW, original_tiles::MONSTER_TILES, false, true) ||
        !original_tiles::make_actor_region_readable(pixels, width, height, original_tiles::PLAYER_BASE_ROW, original_tiles::PLAYER_TILES, true) ||
        (original_tiles::monster_catalog_available && !original_tiles::make_actor_region_readable(pixels, width, height, original_tiles::MONSTER_CATALOG_BASE_ROW, original_tiles::MONSTER_CATALOG_FRAMES, false))) {
        ReleaseDC(nullptr, dc);
        return nullptr;
    }
    void *bits = nullptr;
    const auto result = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (result && bits) {
        std::memcpy(bits, pixels.data(), pixels.size() * sizeof(uint32_t));
    }
    ReleaseDC(nullptr, dc);
    return result;
}

namespace Impl {
graphics_mode change_graphics(graphics_mode arg)
{
    if (current_graphics_mode == arg) {
        return current_graphics_mode;
    }

    BYTE wid, hgt, twid, thgt, ox, oy;
    std::string name;
    std::string name_mask("");

    infGraph.delete_bitmap();
    original_tiles::catalog_available = false;
    original_tiles::monster_catalog_available = false;

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
    } else if (arg == graphics_mode::GRAPHICS_ORIGINAL_TERRAIN) {
        wid = hgt = twid = thgt = 32;
        ox = oy = 0;
        name = "original32.png";
        ANGBAND_GRAF = "original32";
    } else {
        ANGBAND_GRAF = "ascii";
        current_graphics_mode = graphics_mode::GRAPHICS_NONE;
        return current_graphics_mode;
    }

    const auto path = path_build(ANGBAND_DIR_XTRA_GRAF, name);
    infGraph.hBitmap = read_graphic(path);
    if (!infGraph.hBitmap) {
        plog(fmt::format(_("ビットマップ '{}' を読み込めません。", "Cannot read bitmap file '{}'"), name));
        ANGBAND_GRAF = "ascii";
        current_graphics_mode = graphics_mode::GRAPHICS_NONE;
        return current_graphics_mode;
    }

    if (arg == graphics_mode::GRAPHICS_ORIGINAL_TERRAIN) {
        BITMAP dimensions{};
        original_tiles::catalog_available = GetObject(infGraph.hBitmap, static_cast<int>(sizeof(dimensions)), &dimensions) == static_cast<int>(sizeof(dimensions)) &&
                                            dimensions.bmWidth == 512 && dimensions.bmHeight >= original_tiles::CATALOG_NATIVE_HEIGHT;
        original_tiles::monster_catalog_available = original_tiles::catalog_available && dimensions.bmHeight >= original_tiles::MONSTER_CATALOG_NATIVE_HEIGHT;
        if (const auto readable = readable_actor_bitmap(static_cast<HBITMAP>(infGraph.hBitmap))) {
            DeleteObject(infGraph.hBitmap);
            infGraph.hBitmap = readable;
        }
    }

    infGraph.CellWidth = wid;
    infGraph.CellHeight = hgt;
    infGraph.TileWidth = twid;
    infGraph.TileHeight = thgt;
    infGraph.OffsetX = ox;
    infGraph.OffsetY = oy;

    if (name_mask.empty()) {
        current_graphics_mode = arg;
        return arg;
    }

    const auto path_mask = path_build(ANGBAND_DIR_XTRA_GRAF, name_mask);
    infGraph.hBitmapMask = read_graphic(path_mask);
    if (infGraph.hBitmapMask) {
        current_graphics_mode = arg;
        return arg;
    }

    plog(fmt::format(_("ビットマップ '{}' を読み込めません。", "Cannot read bitmap file '{}'"), name_mask));
    ANGBAND_GRAF = "ascii";
    current_graphics_mode = graphics_mode::GRAPHICS_NONE;
    return current_graphics_mode;
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
