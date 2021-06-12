#pragma once

#include "main-win/main-win-cfg-reader.h"
#include "system/h-type.h"

extern concptr ANGBAND_DIR_XTRA_SOUND;
extern CfgData *sound_cfg_data;

void load_sound_prefs(void);
void finalize_sound(void);
errr play_sound(int val);
