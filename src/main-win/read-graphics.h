#pragma once
/*!
 * @file read-graphics.h
 * @brief Windows版固有実装(イメージファイルの読み込み)ヘッダ
 */

#include <windows.h>

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
