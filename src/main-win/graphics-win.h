#pragma once
/*!
 * @file graphics-win.h
 * @brief Windows版固有実装(タイル、イメージファイルの読み込み)ヘッダ
 */

#include "system/h-type.h"

#include <windows.h>

/*
 * Available graphic modes
 */
enum class graphics_mode : byte {
    GRAPHICS_NONE = 0, //!< None
    GRAPHICS_ORIGINAL = 1, //!< Old tiles
    GRAPHICS_ADAM_BOLT = 2, //!< Adam Bolt's tiles
    GRAPHICS_HENGBAND = 3, //!< 新タイル
};

/*!
 * @struct tile_info
 * @brief Information about a tile
 */
struct tile_info {
    HANDLE hBitmap = NULL;
    HANDLE hBitmapMask = NULL;
    BYTE CellWidth = 0;
    BYTE CellHeight = 0;
    BYTE TileWidth = 0;
    BYTE TileHeight = 0;
    INT OffsetX = 0;
    INT OffsetY = 0;

    virtual ~tile_info()
    {
        delete_bitmap();
    }

    void delete_bitmap()
    {
        if (hBitmap) {
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }

        if (hBitmapMask) {
            DeleteObject(hBitmapMask);
            hBitmapMask = NULL;
        }
    }
};

/*!
 * @brief グラフィクスのモード、タイル情報管理
 */
class Graphics {
public:
    Graphics() = default;

    /*!
     * @brief 現在のモードを取得する
     * @return 現在のモード
     */
    graphics_mode get_mode(void);

    /*!
     * @brief 指定モードのタイルに変更する / Change graphics (tile)
     * @param arg graphics_mode
     * @return 変更後のモードを返す。失敗した場合は「なし」のモードを返す。
     */
    graphics_mode change_graphics(graphics_mode arg);

    /*!
     * @brief 現在のタイル情報を取得する
     * @return tile_info
     */
    const tile_info &get_tile_info(void);

    /*!
     * @brief 初期化
     * @details
     * 必要な時に自動的に初期化が行われるため、明示的に行う必要はない。
     * 事前に初期化処理を済ませたい場合に使用すること。
     */
    void init(void);

    /*!
     * @brief 終了処理
     * @details graphicsのリソースを解放する。
     * プロセスの終了前のみ使用すること。
     * 繰り返し初期化と終了処理を行わないこと。
     */
    void finalize(void);

    Graphics(const Graphics &) = delete;
    void operator=(const Graphics &) = delete;
};

/*!
 * グラフィックのモード、タイル情報管理
 */
extern Graphics graphic;

/*
 * Directory names
 */
extern concptr ANGBAND_DIR_XTRA_GRAF;

/*!
 * @brief Creates a GDI bitmap from an image file
 * @details
 * Supported Image File Formats : BMP, ICON, GIF, JPEG, Exif, PNG, TIFF, WMF, and EMF.
 * @param filename an image file name
 * @return bitmap handle
 */
HBITMAP read_graphic(char *filename);
