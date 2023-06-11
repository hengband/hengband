#pragma once

#include "system/angband.h"

#define MAX_HISCORES 999 /*!< スコア情報保存の最大数 / Maximum number of high scores in the high score file */

/*
 * Semi-Portable High Score List Entry (128 bytes) -- BEN
 *
 * All fields listed below are null terminated ascii strings.
 *
 * In addition, the "number" fields are right justified, and
 * space padded, to the full available length (minus the "null").
 *
 * Note that "string comparisons" are thus valid on "pts".
 */
class PlayerType;
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

    void copy_info(const PlayerType &player);
};

extern int highscore_fd;

int highscore_seek(int i);
errr highscore_read(high_score *score);
