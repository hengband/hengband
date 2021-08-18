#pragma once

#include "system/angband.h"

/*
 * Quest constants
 */
#define MIN_RANDOM_QUEST 40 /*<! ランダムクエストを割り当てるクエストIDの開始値 */
#define MAX_RANDOM_QUEST 49 /*<! ランダムクエストを割り当てるクエストIDの終了値 */

/*
 * @brief Quest status
 * @todo 後でenumにする.
 */
#define QUEST_STATUS_UNTAKEN              0 /*!< クエストステータス状態：未発生*/
#define QUEST_STATUS_TAKEN                1 /*!< クエストステータス状態：発生中*/
#define QUEST_STATUS_COMPLETED            2 /*!< クエストステータス状態：達成*/
#define QUEST_STATUS_REWARDED             3 /*!< クエストステータス状態：報酬受け取り前*/
#define QUEST_STATUS_FINISHED             4 /*!< クエストステータス状態：完了*/
#define QUEST_STATUS_FAILED               5 /*!< クエストステータス状態：失敗*/
#define QUEST_STATUS_FAILED_DONE          6 /*!< クエストステータス状態：失敗完了*/
#define QUEST_STATUS_STAGE_COMPLETED      7 /*!< クエストステータス状態：ステージ毎達成*/

/*
 * Quest type
 */
#define QUEST_TYPE_KILL_LEVEL                1 /*!< クエスト目的: 特定のユニークモンスターを倒す */
#define QUEST_TYPE_KILL_ANY_LEVEL            2 /*!< クエスト目的: イベント受託時点でランダムで選ばれた特定のユニークモンスターを倒す */
#define QUEST_TYPE_FIND_ARTIFACT             3 /*!< クエスト目的: 特定のアーティファクトを発見する */
#define QUEST_TYPE_FIND_EXIT                 4 /*!< クエスト目的: 脱出する */
#define QUEST_TYPE_KILL_NUMBER               5 /*!< クエスト目的: モンスターを無差別に特定数倒す */
#define QUEST_TYPE_KILL_ALL                  6 /*!< クエスト目的: エリア中のすべてのモンスターを全て倒す */
#define QUEST_TYPE_RANDOM                    7 /*!< クエスト目的: ランダムクエストとして選ばれたユニーク1体を倒す */
#define QUEST_TYPE_TOWER                     8 /*!< クエスト目的: 複数のエリアの全てのモンスターを倒す */

/*
 * Quest flags
 */
#define QUEST_FLAG_SILENT  0x01 /*!< クエストフラグ: クエスト進行に関する情報表示を抑止する / no messages from completion */
#define QUEST_FLAG_PRESET  0x02 /*!< クエストフラグ: クエストがダンジョン外で発生する / quest is outside the main dungeon */
#define QUEST_FLAG_ONCE    0x04 /*!< クエストフラグ: クエストがフロアを出た時点で完了する / quest is marked finished after leaving */
#define QUEST_FLAG_TOWER   0x08 /*!< クエストフラグ: クエスト:塔の形式で進行する / Tower quest is special */

/*!
 * @brief 該当IDが固定クエストかどうかを判定する / Check is the quest index is "fixed"
 * @param Q_IDX クエストID
 * @return 固定クエストならばTRUEを返す
 */
#define is_fixed_quest_idx(Q_IDX) (((Q_IDX) < MIN_RANDOM_QUEST) || ((Q_IDX) > MAX_RANDOM_QUEST))

#define QUEST_TOWER1 5 /*<! 塔クエスト(第1階層)に割り振るクエストID */
#define QUEST_TOWER2 6 /*<! 塔クエスト(第2階層)に割り振るクエストID */
#define QUEST_TOWER3 7 /*<! 塔クエスト(第3階層)に割り振るクエストID */
#define QUEST_OBERON 8 /*<! オベロン打倒クエストに割り振るクエストID */
#define QUEST_SERPENT 9 /*<! サーペント打倒クエストに割り振るクエストID */

/*!
 * @struct quest_type
 * @brief クエスト情報の構造体 / Structure for the "quests".
 */
typedef struct quest_type {
	int16_t status;          /*!< クエストの進行ステータス / Is the quest taken, completed, finished? */
	int16_t type;              /*!< クエストの種別 / The quest type */

	GAME_TEXT name[60];          /*!< クエスト名 / Quest name */
	DEPTH level;            /*!< 処理階層 / Dungeon level */
	MONRACE_IDX r_idx;      /*!< クエスト対象のモンスターID / Monster race */

	MONSTER_NUMBER cur_num; /*!< 撃破したモンスターの数 / Number killed */
	MONSTER_NUMBER max_num; /*!< 求められるモンスターの撃破数 / Number required */

	KIND_OBJECT_IDX k_idx;              /*!< クエスト対象のアイテムID / object index */
	MONSTER_NUMBER num_mon; /*!< QUEST_TYPE_KILL_NUMBER時の目標撃破数 number of monsters on level */

	BIT_FLAGS flags;             /*!< クエストに関するフラグビット / quest flags */
	DUNGEON_IDX dungeon;           /*!< クエスト対象のダンジョンID / quest dungeon */

	PLAYER_LEVEL complev;           /*!< クリア時プレイヤーレベル / player level (complete) */
	REAL_TIME comptime;          /*!< クリア時ゲーム時間 /  quest clear time*/
} quest_type;

extern quest_type *quest;
extern QUEST_IDX max_q_idx;
extern char quest_text[10][80];
extern int quest_text_line;
extern int leaving_quest;

typedef struct object_type object_type;
typedef struct player_type player_type;
void determine_random_questor(player_type *player_ptr, quest_type *q_ptr);
void record_quest_final_status(quest_type *q_ptr, PLAYER_LEVEL lev, int16_t stat);
void complete_quest(player_type *player_ptr, QUEST_IDX quest_num);
void check_find_art_quest_completion(player_type *player_ptr, object_type *o_ptr);
void quest_discovery(QUEST_IDX q_idx);
QUEST_IDX quest_number(player_type *player_ptr, DEPTH level);
QUEST_IDX random_quest_number(player_type *player_ptr, DEPTH level);
void leave_quest_check(player_type *player_ptr);
void leave_tower_check(player_type *player_ptr);
void exe_enter_quest(player_type *player_ptr, QUEST_IDX quest_idx);
void do_cmd_quest(player_type *player_ptr);
