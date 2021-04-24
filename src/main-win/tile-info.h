/*!
 * @file tile-info.h
 * @brief Windows版固有(タイル情報)ヘッダ
 */

#include <windows.h>

/*!
 * @struct tile_info
 * @brief Information about a tile
 */
struct tile_info {
    HANDLE hBitmap = NULL;
    BYTE CellWidth;
    BYTE CellHeight;
    BYTE TileWidth;
    BYTE TileHeight;
    INT OffsetX;
    INT OffsetY;

    void delete_bitmap() {
        if (hBitmap) {
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }
};
