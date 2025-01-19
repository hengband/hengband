#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <array>

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

constexpr std::array<int, 9> ddd = { { 2, 8, 6, 4, 3, 1, 9, 7, 5 } }; //!< キーパッドの方向 (南から反時計回り順).
constexpr std::array<int, 10> ddx = { { 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 } }; //!< dddで定義した順にベクトルのX軸成分.
constexpr std::array<int, 10> ddy = { { 0, 1, 1, 1, 0, 0, 0, -1, -1, -1 } }; //!< dddで定義した順にベクトルのY軸成分.
constexpr std::array<int, 9> ddx_ddd = { { 0, 0, 1, -1, 1, -1, 1, -1, 0 } }; //!< ddd越しにベクトルのX軸成分.
constexpr std::array<int, 9> ddy_ddd = { { 1, -1, 0, 0, 1, 1, -1, -1, 0 } }; //!< ddd越しにベクトルのY軸成分.

//! 下方向から反時計回りに8方向への方向ベクトル配列
constexpr std::array<Pos2DVec, 8> CCW_DD = {
    { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } }
};

class PlayerType;
DIRECTION coords_to_dir(PlayerType *player_ptr, POSITION y, POSITION x);
int distance(const Pos2D &pos1, const Pos2D &pos2);
Pos2D mmove2(const Pos2D &pos_orig, const Pos2D &pos1, const Pos2D &pos2);
bool player_can_see_bold(PlayerType *player_ptr, POSITION y, POSITION x);

class MonsterEntity;
bool is_seen(PlayerType *player_ptr, MonsterEntity *m_ptr);
