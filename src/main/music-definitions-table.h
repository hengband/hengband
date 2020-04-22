#pragma once

#define MUSIC_BASIC_DEFAULT    0
#define MUSIC_BASIC_GAMEOVER   1
#define MUSIC_BASIC_EXIT       2
#define MUSIC_BASIC_TOWN       3
#define MUSIC_BASIC_FIELD1     4
#define MUSIC_BASIC_FIELD2     5
#define MUSIC_BASIC_FIELD3     6
#define MUSIC_BASIC_DUN_LOW    7
#define MUSIC_BASIC_DUN_MED    8
#define MUSIC_BASIC_DUN_HIGH   9
#define MUSIC_BASIC_DUN_FEEL1 10
#define MUSIC_BASIC_DUN_FEEL2 11
#define MUSIC_BASIC_WINNER    12
#define MUSIC_BASIC_BUILD     13
#define MUSIC_BASIC_WILD      14
#define MUSIC_BASIC_QUEST     15
#define MUSIC_BASIC_ARENA     16
#define MUSIC_BASIC_BATTLE    17
#define MUSIC_BASIC_QUEST_CLEAR 18
#define MUSIC_BASIC_FINAL_QUEST_CLEAR 19
#define MUSIC_BASIC_AMBUSH    20
#define MUSIC_BASIC_MAX       21 /*!< BGM定義の最大数 */

extern const concptr angband_music_basic_name[MUSIC_BASIC_MAX];
