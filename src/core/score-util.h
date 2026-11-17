#pragma once

#include "system/angband.h"
#include <cstdint>
#include <string>
#include <string_view>

constexpr auto MAX_HISCORES = 999; /*!< スコア情報保存の最大数 / Maximum number of high scores in the high score file */

class PlayerType;

/*!
 * @brief スコアの記録 1 件
 * @details 文字列の欄は、スコアファイルと同じく UTF-8 で持つ。内部の文字コードへは表示するときに to_sys() で変換する
 * (変換できない文字があっても、ファイルを書き直したときに元の文字が失われないようにするため)
 */
struct HighScore {
    std::string version; //!< 記録したときの変愚蛮怒のバージョン
    int64_t points = 0; //!< スコア
    int64_t gold = 0; //!< 所持金
    int64_t turns = 0; //!< ターン数
    std::string date; //!< 日付 (記録したときは "YYYY-MM-DD"、スコアの予想では「今日」。前の版から移した記録は "MM/DD/YY" のこともある)
    std::string name; //!< 名前
    int64_t uid = 0; //!< ユーザー ID
    std::string sex; //!< 性別 ("m" か "f")
    int race = 0; //!< 種族
    int pclass = 0; //!< 職業
    int personality = 0; //!< 性格
    int level = 0; //!< 現在のレベル
    int dungeon_level = 0; //!< 現在の階
    int max_level = 0; //!< 最高のレベル
    int max_dungeon_level = 0; //!< 最高の階
    std::string died_from; //!< 死因 ("yet" はまだ生きている、"ripe" は勝利の後の引退、"Seppuku" は勝利の後の切腹)

    static HighScore create(PlayerType *player_ptr);
    static std::string from_sys(std::string_view str);
    static std::string to_sys(std::string_view str);
};

/*
 * 前の版のスコアファイル (scores.raw) の 1 件。scores.json への移し替えにだけ使う
 *
 * Semi-Portable High Score List Entry (128 bytes) -- BEN
 *
 * All fields listed below are null terminated ascii strings.
 *
 * In addition, the "number" fields are right justified, and
 * space padded, to the full available length (minus the "null").
 *
 * Note that "string comparisons" are thus valid on "pts".
 */
struct high_score {
    GAME_TEXT what[8]{}; /* Version info (string) */
    GAME_TEXT pts[10]{}; /* Total Score (number) */
    GAME_TEXT gold[10]{}; /* Total Gold (number) */
    GAME_TEXT turns[10]{}; /* Turns Taken (number) */
    GAME_TEXT day[10]{}; /* Time stamp (string) */
    GAME_TEXT who[16]{}; /* Player Name (string) */
    GAME_TEXT uid[8]{}; /* Player UID (number) */
    GAME_TEXT sex[2]{}; /* Player Sex (string) */
    GAME_TEXT p_r[3]{}; /* Player Race (number) */
    GAME_TEXT p_c[3]{}; /* Player Class (number) */
    GAME_TEXT p_a[3]{}; /* Player Seikaku (number) */

    GAME_TEXT cur_lev[4]{}; /* Current Player Level (number) */
    GAME_TEXT cur_dun[4]{}; /* Current Dungeon Level (number) */
    GAME_TEXT max_lev[4]{}; /* Max Player Level (number) */
    GAME_TEXT max_dun[4]{}; /* Max Dungeon Level (number) */

    GAME_TEXT how[40]{}; /* Method of death (string) */

    HighScore to_score() const;
};
