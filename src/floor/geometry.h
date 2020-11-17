#pragma once

#include "system/angband.h"
#include "system/monster-type-definition.h"

typedef struct player_type player_type;

extern const POSITION ddd[9];
extern const POSITION ddx[10];
extern const POSITION ddy[10];
extern const POSITION ddx_ddd[9];
extern const POSITION ddy_ddd[9];
extern const POSITION cdd[8];
extern const POSITION ddx_cdd[8];
extern const POSITION ddy_cdd[8];

extern DIRECTION coords_to_dir(player_type *creature_ptr, POSITION y, POSITION x);

extern POSITION distance(POSITION y1, POSITION x1, POSITION y2, POSITION x2);

extern void mmove2(POSITION *y, POSITION *x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);

extern bool player_can_see_bold(player_type *creature_ptr, POSITION y, POSITION x);

/*!
 * @brief 視界及び光源の過渡処理配列サイズ / Maximum size of the "temp" array
 * @details We must be as large as "VIEW_MAX" and "LITE_MAX" for proper functioning
 * of "update_view()" and "update_lite()".  We must also be as large as the
 * largest illuminatable room, but no room is larger than 800 grids.  We
 * must also be large enough to allow "good enough" use as a circular queue,
 * to calculate monster flow, but note that the flow code is "paranoid".
 */
#define TEMP_MAX 2298

//!< 対象グリッドの一覧をまとめる構造体
typedef struct pos_list {
	POSITION_IDX n; //!< Array of grids for use by various functions (see grid.c")
	POSITION y[TEMP_MAX];
	POSITION x[TEMP_MAX];
} pos_list;

bool is_seen(player_type *creature_ptr, monster_type *m_ptr);
