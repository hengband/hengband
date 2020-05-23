#pragma once

#include "system/angband.h"

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
typedef struct
{
	POSITION_IDX n; //!< Array of grids for use by various functions (see grid.c")
	POSITION y[TEMP_MAX];
	POSITION x[TEMP_MAX];
} pos_list;

//!< ターゲット指定構造体
typedef struct
{
	DIRECTION dir;
	POSITION y;
	POSITION x;
} target_dir;

/*
 * Simple structure to hold a map location
 */
typedef struct coord coord;

struct coord
{
	POSITION y;
	POSITION x;
};

/*
 * Is the monster seen by the player?
 */
#define is_seen(A) \
	((bool)((A)->ml && (!ignore_unview || p_ptr->phase_out || \
	 (player_can_see_bold(p_ptr, (A)->fy, (A)->fx) && projectable(p_ptr, p_ptr->y, p_ptr->x, (A)->fy, (A)->fx)))))

/*
 * todo is_seen() の関数マクロをバラそうとしたがインクルード関係のコンパイルエラーで失敗
 * Is the monster seen by the player?
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 個々のモンスターへの参照ポインタ
 * @return 個々のモンスターがプレーヤーが見えたらTRUE
 */

/*
extern bool is_seen(player_type *creature_ptr, monster_type *m_ptr);
bool is_seen(player_type *creature_ptr, monster_type *m_ptr)
{
	bool is_inside_view = !ignore_unview;
	is_inside_view |= creature_ptr->phase_out;
	is_inside_view |= player_can_see_bold(creature_ptr, m_ptr->fy, m_ptr->fx) &&
		projectable(creature_ptr, creature_ptr->y, creature_ptr->x, m_ptr->fy, m_ptr->fx);
	return m_ptr->ml && is_inside_view;
}

*/
