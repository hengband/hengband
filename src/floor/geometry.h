#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

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
struct pos_list {
    POSITION_IDX n; //!< Array of grids for use by various functions (see grid.c")
    POSITION y[TEMP_MAX];
    POSITION x[TEMP_MAX];
};

extern const int ddd[9];
extern const int ddx[10];
extern const int ddy[10];
extern const int ddx_ddd[9];
extern const int ddy_ddd[9];
extern const int cdd[8];
extern const int ddx_cdd[8];
extern const int ddy_cdd[8];

class PlayerType;
DIRECTION coords_to_dir(PlayerType *player_ptr, POSITION y, POSITION x);
POSITION distance(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
Pos2D mmove2(const Pos2D &pos_orig, const Pos2D &pos1, const Pos2D &pos2);
bool player_can_see_bold(PlayerType *player_ptr, POSITION y, POSITION x);

class MonsterEntity;
bool is_seen(PlayerType *player_ptr, MonsterEntity *m_ptr);
