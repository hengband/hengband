#include "game-option/special-options.h"

byte hitpoint_warn; /* Hitpoint warning (0 to 9) */
byte mana_warn; /* Mana color (0 to 9) */
byte delay_factor; /* Delay factor (0 to 9) */
bool autosave_l; /* Autosave before entering new levels */
bool autosave_t; /* Timed autosave */
int16_t autosave_freq; /* Autosave frequency */
bool use_sound; /* The "sound" mode is enabled */
bool use_music; /* The "music" mode is enabled */
bool use_graphics; /* The "graphics" mode is enabled */
bool use_bigtile = false;
