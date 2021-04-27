#pragma once
/*!
 * @file graphics-win.h
 * @brief Windows版固有実装(イメージファイルの読み込み)ヘッダ
 */

#include <windows.h>

/*
 * Available graphic modes
 */
enum graphics_mode {
    GRAPHICS_NONE = 0,
    GRAPHICS_ORIGINAL = 1,
    GRAPHICS_ADAM_BOLT = 2,
    GRAPHICS_HENGBAND = 3,
};

/*!
 * @struct tile_info
 * @brief Information about a tile
 */
struct tile_info {
    HANDLE hBitmap = NULL;
    BYTE CellWidth = 0;
    BYTE CellHeight = 0;
    BYTE TileWidth = 0;
    BYTE TileHeight = 0;
    INT OffsetX = 0;
    INT OffsetY = 0;

    void delete_bitmap()
    {
        if (hBitmap) {
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }
};

/*!
 * @brief Cleans up resources used by graphics
 */
void finalize_graphics();

/*!
 * @brief Creates a GDI bitmap from an image file
 * @details
 * Supported Image File Formats : BMP, ICON, GIF, JPEG, Exif, PNG, TIFF, WMF, and EMF.
 * @param filename an image file name
 * @return bitmap handle
 */
HBITMAP read_graphic(char *filename);
