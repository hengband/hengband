#include "game-option/special-options.h"

byte hitpoint_warn = 4; /* Hitpoint warning (0 to 9) */
byte mana_warn = 3; /* Mana color (0 to 9) */
byte delay_factor = 1; /* Delay factor (0 to 9) */
bool autosave_l; /* Autosave before entering new levels */
bool autosave_t; /* Timed autosave */
s16b autosave_freq; /* Autosave frequency */
bool use_sound; /* The "sound" mode is enabled */
bool use_music; /* The "music" mode is enabled */
bool use_graphics; /* The "graphics" mode is enabled */
bool use_bigtile = FALSE;
