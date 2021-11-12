#pragma once

#include "system/angband.h"

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

extern const POSITION ddd[9];
extern const POSITION ddx[10];
extern const POSITION ddy[10];
extern const POSITION ddx_ddd[9];
extern const POSITION ddy_ddd[9];
extern const POSITION cdd[8];
extern const POSITION ddx_cdd[8];
extern const POSITION ddy_cdd[8];

class PlayerType;
DIRECTION coords_to_dir(PlayerType *player_ptr, POSITION y, POSITION x);
POSITION distance(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
void mmove2(POSITION *y, POSITION *x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
bool player_can_see_bold(PlayerType *player_ptr, POSITION y, POSITION x);

struct monster_type;
bool is_seen(PlayerType *player_ptr, monster_type *m_ptr);
