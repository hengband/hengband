#pragma once

#include "system/h-type.h"

#include "main/music-definitions-table.h"

#include <windows.h>

#define SAMPLE_MUSIC_MAX 16
extern concptr music_file[MUSIC_BASIC_MAX][SAMPLE_MUSIC_MAX];
// TODO マジックナンバー除去
extern concptr dungeon_music_file[1000][SAMPLE_MUSIC_MAX];
extern concptr town_music_file[1000][SAMPLE_MUSIC_MAX];
extern concptr quest_music_file[1000][SAMPLE_MUSIC_MAX];
extern concptr ANGBAND_DIR_XTRA_MUSIC;

namespace main_win_music {
void load_music_prefs(DUNGEON_IDX max_d_idx, QUEST_IDX max_q_idx);
void stop_music(void);
errr play_music(int type, int val);
void on_mci_notify(WPARAM wFlags, LONG lDevID);
}
