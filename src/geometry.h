#pragma once

DIRECTION coords_to_dir(POSITION y, POSITION x);

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
