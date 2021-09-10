#pragma once

#include "player-info/class-types.h"
#include "system/angband.h"
#include "util/flag-group.h"

#define MAX_BOUNTY 20

/*!
 * @brief 世界情報構造体
 */
struct world_type {

    POSITION max_wild_x{}; /*!< Maximum size of the wilderness */
    POSITION max_wild_y{}; /*!< Maximum size of the wilderness */
    GAME_TURN game_turn{}; /*!< 画面表示上のゲーム時間基準となるターン / Current game turn */
    GAME_TURN game_turn_limit{}; /*!< game_turnの最大値 / Limit of game_turn */
    GAME_TURN dungeon_turn{}; /*!< NASTY生成の計算に関わる内部ターン値 / Game turn in dungeon */
    GAME_TURN dungeon_turn_limit{}; /*!< dungeon_turnの最大値 / Limit of game_turn in dungeon */
    GAME_TURN arena_start_turn{}; /*!< 闘技場賭博の開始ターン値 */
    uint32_t start_time{};
    uint16_t noscore{}; /* Cheating flags */
    uint16_t total_winner{}; /* Total winner */

    MONSTER_IDX timewalk_m_idx{}; /*!< 現在時間停止を行っているモンスターのID */

    MONRACE_IDX bounty_r_idx[MAX_BOUNTY]{};
    MONSTER_IDX today_mon{}; //!< 実際の日替わり賞金首

    uint32_t play_time{}; /*!< 実プレイ時間 */

    uint32_t seed_flavor{}; /* Hack -- consistent object colors */
    uint32_t seed_town{}; /* Hack -- consistent town layout */

    bool is_loading_now{}; /*!< ロード処理中フラグ...ロード直後にcalc_bonus()時の徳変化、及びsanity_blast()による異常を抑止する */

    byte h_ver_major{}; //!< 変愚蛮怒バージョン(メジャー番号) / Hengband version (major ver.)
    byte h_ver_minor{}; //!< 変愚蛮怒バージョン(マイナー番号) / Hengband version (minor ver.)
    byte h_ver_patch{}; //!< 変愚蛮怒バージョン(パッチ番号) / Hengband version (patch ver.)
    byte h_ver_extra{}; //!< 変愚蛮怒バージョン(エクストラ番号) / Hengband version (extra ver.)

    byte sf_extra{}; //!< セーブファイルエンコードキー(XOR)

    uint32_t sf_system{}; //!< OS情報 / OS information
    uint32_t sf_when{}; //!< 作成日時 / Created Date
    uint16_t sf_lives{}; //!< このセーブファイルで何人プレイしたか / Number of past "lives" with this file
    uint16_t sf_saves{}; //!< 現在のプレイで何回セーブしたか / Number of "saves" during this life
    uint32_t sf_play_time{}; //!< このセーブファイルで遊んだ合計のプレイ時間
    FlagGroup<player_class_type, MAX_CLASS> sf_winner{}; //!< このセーブファイルで*勝利*した職業
    FlagGroup<player_class_type, MAX_CLASS> sf_retired{}; //!< このセーブファイルで引退した職業

    bool character_generated{}; /* The character exists */
    bool character_dungeon{}; /* The character has a dungeon */
    bool character_loaded{}; /* The character was loaded from a savefile */
    bool character_saved{}; /* The character was just saved to a savefile */

    byte character_icky_depth{}; /* The game is in an icky full screen mode */
    bool character_xtra{}; /* The game is in an icky startup mode */

    bool creating_savefile{}; /* New savefile is currently created */

    bool wizard{}; /* This world under wizard mode */

    OBJECT_IDX max_o_idx{}; /*!< Maximum number of objects in the level */
    MONSTER_IDX max_m_idx{}; /*!< Maximum number of monsters in the level */

    DUNGEON_IDX max_d_idx{};
};

extern world_type *current_world_ptr;

struct player_type;
bool is_daytime(void);
void extract_day_hour_min(player_type *player_ptr, int *day, int *hour, int *min);
void update_playtime(void);
void add_winner_class(player_class_type c);
void add_retired_class(player_class_type c);
bool is_winner_class(player_class_type c);
bool is_retired_class(player_class_type c);
