#pragma once

/*!
 * @brief プレイヤー用光源処理配列サイズ / Maximum size of the "lite" array (see "grid.c")
 * @details Note that the "lite radius" will NEVER exceed 14, and we would
 * never require more than 581 entries in the array for circular "lite".
 */
#define LITE_MAX 600

/*!
 * @brief モンスター用光源処理配列サイズ / Maximum size of the "mon_lite" array (see ">grid.c")
 * @details Note that the "view radius" will NEVER exceed 20, monster illumination
 * flags are dependent on CAVE_VIEW, and even if the "view" was octagonal,
 * we would never require more than 1520 entries in the array.
 */
#define MON_LITE_MAX 1536

/*!
 * @brief 視界処理配列サイズ / Maximum size of the "view" array
 * @details Note that the "view radius" will NEVER exceed 20, and even if the "view"
 * was octagonal, we would never require more than 1520 entries in the array.
 */
#define VIEW_MAX 1536

/*!
 * @brief 再描画処理用配列サイズ / Maximum size of the "redraw" array
 * @details We must be large for proper functioning of delayed redrawing.
 * We must also be as large as two times of the largest view area.
 * Note that maximum view grids are 1149 entries.
 */
#define REDRAW_MAX 2298
