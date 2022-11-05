#pragma once

#include "main-win/main-win-cfg-reader.h"
#include "system/h-type.h"

#include "main/music-definitions-table.h"

#include <array>
#include <windows.h>

extern bool use_pause_music_inactive;
extern concptr ANGBAND_DIR_XTRA_MUSIC;
extern CfgData *music_cfg_data;

namespace main_win_music {
void load_music_prefs();
errr stop_music(void);
errr play_music(int type, int val, int volume);
errr play_music_scene(int val, int volume);
void pause_music(void);
void set_music_volume(int volume);
void resume_music(void);
void on_mci_notify(WPARAM wFlags, LONG lDevID);

/*! 音量 100%,90%,…,10% それぞれに割り当てる実際の値(音量の指定可能範囲:0～1000) */
constexpr std::array<int, 10> VOLUME_TABLE = { { 1000, 800, 600, 450, 350, 250, 170, 100, 50, 20 } };
}
