#pragma once

typedef struct {
	POSITION max_wild_x; /*!< Maximum size of the wilderness */
	POSITION max_wild_y; /*!< Maximum size of the wilderness */
	GAME_TURN game_turn;			/*!< 画面表示上のゲーム時間基準となるターン / Current game turn */
	GAME_TURN game_turn_limit;		/*!< game_turnの最大値 / Limit of game_turn */
	GAME_TURN dungeon_turn;			/*!< NASTY生成の計算に関わる内部ターン値 / Game current_world_ptr->game_turn in dungeon */
	GAME_TURN dungeon_turn_limit;	/*!< dungeon_turnの最大値 / Limit of game_turn in dungeon */
	MONSTER_IDX timewalk_m_idx;     /*!< 現在時間停止を行っているモンスターのID */

	MONRACE_IDX bounty_r_idx[MAX_KUBI];

	u32b play_time; /*!< 実プレイ時間 */

	u32b seed_flavor;		/* Hack -- consistent object colors */
	u32b seed_town;			/* Hack -- consistent town layout */

} world_type;

extern bool is_daytime(void);
extern void extract_day_hour_min(int *day, int *hour, int *min);
extern world_type *current_world_ptr;