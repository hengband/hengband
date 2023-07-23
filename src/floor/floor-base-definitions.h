#pragma once

/*!
 * @brief ダンジョンの最深層 / Maximum dungeon level.
 * @details
 * The player can never reach this level
 * in the dungeon, and this value is used for various calculations
 * involving object and monster creation.  It must be at least 100.
 * Setting it below 128 may prevent the creation of some objects.
 */
constexpr int MAX_DEPTH = 128;

/*!
 * @brief 基本的なブロック数単位(垂直方向)
 * Number of grids in each block (vertically) Probably hard-coded to 11
 */
constexpr int BLOCK_HGT = 11;

/*!
 * @brief 基本的なブロック数単位(水平方向)
 * Number of grids in each block (horizontally) Probably hard-coded to 11
 */
constexpr int BLOCK_WID = 11;

/*!
 * @brief 表示上の基本的なブロック単位(垂直方向、PANEL_HGTの倍数で設定すること)
 * Number of grids used to display the dungeon (vertically). Must be a multiple of 11, probably hard-coded to 22.
 */
constexpr int SCREEN_HGT = 22;

/*!
 * @brief 表示上の基本的なブロック単位(水平方向、PANEL_WIDの倍数で設定すること)
 * Number of grids used to display the dungeon (horizontally). Must be a multiple of 33, probably hard-coded to 66.
 */
constexpr int SCREEN_WID = 66;

/*!
 * @brief 表示上のダンジョンの最大垂直サイズ(SCREEN_HGTの3倍が望ましい)
 * Maximum dungeon height in grids, must be a multiple of SCREEN_HGT, probably hard-coded to SCREEN_HGT * 3.
 */
constexpr int MAX_HGT = 66;

/*!
 * @brief 表示上のダンジョンの最大水平サイズ(SCREEN_WIDの3倍が望ましい)
 * Maximum dungeon width in grids, must be a multiple of SCREEN_WID, probably hard-coded to SCREEN_WID * 3.
 */
constexpr int MAX_WID = 198;
