#pragma once

extern const POSITION ddd[9];
extern const POSITION ddx[10];
extern const POSITION ddy[10];
extern const POSITION ddx_ddd[9];
extern const POSITION ddy_ddd[9];
extern const POSITION cdd[8];
extern const POSITION ddx_cdd[8];
extern const POSITION ddy_cdd[8];

extern DIRECTION coords_to_dir(POSITION y, POSITION x);

/*
 * project()関数に用いられる、遠隔攻撃特性ビットフラグ / Bit flags for the "project()" function
 */
#define PROJECT_JUMP        0x0001 /*!< 遠隔攻撃特性: 発動者からの軌跡を持たず、指定地点に直接発生する(予め置いたトラップ、上空からの発生などのイメージ) / Jump directly to the target location (this is a hack) */
#define PROJECT_BEAM        0x0002 /*!< 遠隔攻撃特性: ビーム範囲を持つ。 / Work as a beam weapon (affect every grid passed through) */
#define PROJECT_THRU        0x0004 /*!< 遠隔攻撃特性: 目標地点に到達しても射程と遮蔽の限り引き延ばす。 / Continue "through" the target (used for "bolts"/"beams") */
#define PROJECT_STOP        0x0008 /*!< 遠隔攻撃特性: 道中にプレイヤーかモンスターがいた時点で到達地点を更新して停止する(壁や森はPROJECT_DISIがない限り最初から貫通しない) */
#define PROJECT_GRID        0x0010 /*!< 遠隔攻撃特性: 射程内の地形に影響を及ぼす / Affect each grid in the "blast area" in some way */
#define PROJECT_ITEM        0x0020 /*!< 遠隔攻撃特性: 射程内のアイテムに影響を及ぼす / Affect each object in the "blast area" in some way */
#define PROJECT_KILL        0x0040 /*!< 遠隔攻撃特性: 射程内のモンスターに影響を及ぼす / Affect each monster in the "blast area" in some way */
#define PROJECT_HIDE        0x0080 /*!< 遠隔攻撃特性: / Hack -- disable "visual" feedback from projection */
#define PROJECT_DISI        0x0100 /*!< 遠隔攻撃特性: / Disintegrate non-permanent features */
#define PROJECT_PLAYER      0x0200 /*!< 遠隔攻撃特性: / Main target is player (used for riding player) */
#define PROJECT_AIMED       0x0400 /*!< 遠隔攻撃特性: / Target is only player or monster, so don't affect another. Depend on PROJECT_PLAYER. (used for minimum (rad == 0) balls on riding player) */
#define PROJECT_REFLECTABLE 0x0800 /*!< 遠隔攻撃特性: 反射可能(ボルト系魔法に利用) / Refrectable spell attacks (used for "bolts") */
#define PROJECT_NO_HANGEKI  0x1000 /*!< 遠隔攻撃特性: / Avoid counter attacks of monsters */
#define PROJECT_PATH        0x2000 /*!< 遠隔攻撃特性: / Only used for printing project path */
#define PROJECT_FAST        0x4000 /*!< 遠隔攻撃特性: / Hide "visual" of flying bolts until blast */
#define PROJECT_LOS         0x8000 /*!< 遠隔攻撃特性: /  */
extern sint project_path(u16b *gp, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flg);

extern POSITION distance(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern bool los(POSITION y1, POSITION x1, POSITION y2, POSITION x2);

extern bool projectable(POSITION y1, POSITION x1, POSITION y2, POSITION x2);
extern void scatter(POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
extern void mmove2(POSITION *y, POSITION *x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);

extern bool player_can_see_bold(POSITION y, POSITION x);
extern bool no_lite(void);

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


