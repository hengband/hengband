#pragma once

#include "main-win/main-win-cfg-reader.h"
#include "system/h-type.h"

#include "main/music-definitions-table.h"

#include <windows.h>

extern concptr ANGBAND_DIR_XTRA_MUSIC;
extern CfgData *music_cfg_data;

namespace main_win_music {
void load_music_prefs();
errr stop_music(void);
errr play_music(int type, int val);
errr play_music_scene(int val);
void on_mci_notify(WPARAM wFlags, LONG lDevID);
}
