#pragma once

#include "market/bounty-type-definition.h"
#include "player-info/class-types.h"
#include "system/angband.h"
#include "util/elapsed-time.h"
#include "util/flag-group.h"
#include <tuple>

constexpr auto MAX_BOUNTY = 20;

/*!
 * @brief 世界情報構造体
 */
enum term_color_type : unsigned char;
enum class PlayerRaceType;
class MonraceDefinition;
class AngbandWorld {
public:
    ~AngbandWorld() = default;
    AngbandWorld(AngbandWorld &&) = delete;
    AngbandWorld(const AngbandWorld &) = delete;
    AngbandWorld &operator=(const AngbandWorld &) = delete;
    AngbandWorld &operator=(AngbandWorld &&) = delete;
    static AngbandWorld &get_instance();

    POSITION max_wild_x{}; /*!< Maximum size of the wilderness */
    POSITION max_wild_y{}; /*!< Maximum size of the wilderness */
    GAME_TURN game_turn{}; /*!< 画面表示上のゲーム時間基準となるターン / Current game turn */
    GAME_TURN dungeon_turn{}; /*!< NASTY生成の計算に関わる内部ターン値 / Game turn in dungeon */
    GAME_TURN dungeon_turn_limit{}; /*!< dungeon_turnの最大値 / Limit of game_turn in dungeon */
    GAME_TURN arena_start_turn{}; /*!< 闘技場賭博の開始ターン値 */
    uint16_t noscore{}; /* Cheating flags */
    uint16_t total_winner{}; /* Total winner */

    MONSTER_IDX timewalk_m_idx{}; /*!< 現在時間停止を行っているモンスターのID */

    bounty_type bounties[MAX_BOUNTY]{}; //!< 賞金首ユニーク
    bool knows_daily_bounty{}; //!< 日替わり賞金首を知っているか否か
    MonraceId today_mon{}; //!< 実際の日替わり賞金首

    ElapsedTime play_time{}; /*!< 実プレイ時間 */

    bool is_loading_now{}; /*!< ロード処理中フラグ...ロード直後にcalc_bonus()時の徳変化、及びsanity_blast()による異常を抑止する */

    uint32_t sf_system{}; //!< OS情報 / OS information
    uint32_t sf_when{}; //!< 作成日時 / Created Date
    uint16_t sf_lives{}; //!< このセーブファイルで何人プレイしたか / Number of past "lives" with this file
    uint16_t sf_saves{}; //!< 現在のプレイで何回セーブしたか / Number of "saves" during this life
    uint32_t sf_play_time{}; //!< このセーブファイルで遊んだ合計のプレイ時間
    EnumClassFlagGroup<PlayerClassType> sf_winner{}; //!< このセーブファイルで*勝利*した職業
    EnumClassFlagGroup<PlayerClassType> sf_retired{}; //!< このセーブファイルで引退した職業

    bool character_generated{}; /* The character exists */
    bool character_dungeon{}; /* The character has a dungeon */
    bool character_loaded{}; /* The character was loaded from a savefile */
    bool character_saved{}; /* The character was just saved to a savefile */

    byte character_icky_depth{}; /* The game is in an icky full screen mode */
    bool character_xtra{}; /* The game is in an icky startup mode */

    bool creating_savefile{}; /* New savefile is currently created */

    bool wizard{}; /* This world under wizard mode */

    bool is_wild_mode() const;
    void set_wild_mode(bool new_wild_mode);
    void set_arena(const bool new_status);
    bool get_arena() const;
    std::tuple<int, int, int> extract_date_time(PlayerRaceType start_race) const;
    bool is_daytime() const;
    void add_winner_class(PlayerClassType c);
    void add_retired_class(PlayerClassType c);
    term_color_type get_birth_class_color(PlayerClassType c) const;
    MonraceDefinition &get_today_bounty();
    const MonraceDefinition &get_today_bounty() const;
    bool is_player_true_winner() const;
    void pass_game_turn_by_stay();
    std::string format_real_playtime() const;
    void set_gametime();

private:
    AngbandWorld() = default;
    static AngbandWorld instance;

    bool is_out_arena = false; // アリーナ外部にいる時だけtrue.
    bool wild_mode = false;

    bool is_winner_class(PlayerClassType c) const;
    bool is_retired_class(PlayerClassType c) const;
};
