#pragma once

#include "system/h-type.h"

#include "main/sound-definitions-table.h"

#define SAMPLE_SOUND_MAX 16
extern concptr sound_file[SOUND_MAX][SAMPLE_SOUND_MAX];
extern concptr ANGBAND_DIR_XTRA_SOUND;

void load_sound_prefs(void);
errr play_sound(int val);
