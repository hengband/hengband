#pragma once
#include "bldg.h"

typedef struct {

	POSITION max_wild_x; /*!< Maximum size of the wilderness */
	POSITION max_wild_y; /*!< Maximum size of the wilderness */
	GAME_TURN game_turn;			/*!< 画面表示上のゲーム時間基準となるターン / Current game turn */
	GAME_TURN game_turn_limit;		/*!< game_turnの最大値 / Limit of game_turn */
	GAME_TURN dungeon_turn;			/*!< NASTY生成の計算に関わる内部ターン値 / Game current_world_ptr->game_turn in dungeon */
	GAME_TURN dungeon_turn_limit;	/*!< dungeon_turnの最大値 / Limit of game_turn in dungeon */
	GAME_TURN arena_start_turn;		/*!< 闘技場賭博の開始ターン値 */
	u32b start_time;

	MONSTER_IDX timewalk_m_idx;     /*!< 現在時間停止を行っているモンスターのID */

	MONRACE_IDX bounty_r_idx[MAX_KUBI];

	u32b play_time; /*!< 実プレイ時間 */

	u32b seed_flavor;		/* Hack -- consistent object colors */
	u32b seed_town;			/* Hack -- consistent town layout */

	bool is_loading_now;	/*!< ロード処理中フラグ...ロード直後にcalc_bonus()時の徳変化、及びsanity_blast(p_ptr, )による異常を抑止する */

	/*
	 * Savefile version
	 */
	byte h_ver_major; /* Savefile version for Hengband 1.1.1 and later */
	byte h_ver_minor;
	byte h_ver_patch;
	byte h_ver_extra;

	byte sf_extra;		/* Savefile's encoding key */

	byte z_major;           /* Savefile version for Hengband */
	byte z_minor;
	byte z_patch;

	/*
	 * Savefile information
	 */
	u32b sf_system;			/* Operating system info */
	u32b sf_when;			/* Time when savefile created */
	u16b sf_lives;			/* Number of past "lives" with this file */
	u16b sf_saves;			/* Number of "saves" during this life */

	bool character_generated;	/* The character exists */
	bool character_dungeon;		/* The character has a dungeon */
	bool character_loaded;		/* The character was loaded from a savefile */
	bool character_saved;		/* The character was just saved to a savefile */

	bool character_icky;		/* The game is in an icky full screen mode */
	bool character_xtra;		/* The game is in an icky startup mode */

	bool creating_savefile;		/* New savefile is currently created */

	bool wizard;		  /* This world under wizard mode */


} world_type;

extern bool is_daytime(void);
extern void extract_day_hour_min(int *day, int *hour, int *min);
extern world_type *current_world_ptr;
