#pragma once

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

extern void scatter(POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
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
	 (player_can_see_bold(p_ptr, (A)->fy, (A)->fx) && projectable(p_ptr->current_floor_ptr, p_ptr->y, p_ptr->x, (A)->fy, (A)->fx)))))

