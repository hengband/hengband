#include "game-option/runtime-arguments.h"

bool arg_sound; /* Command arg -- Request special sounds */
int arg_sound_volume_table_index;
bool arg_music; /* Command arg -- Request special musics */
int arg_music_volume_table_index;
byte arg_graphics; /* Command arg -- Request graphics mode */
bool arg_monochrome; /* Command arg -- Request monochrome mode */
bool arg_force_original; /* Command arg -- Request original keyset */
bool arg_force_roguelike; /* Command arg -- Request roguelike keyset */
bool arg_bigtile = false; /* Command arg -- Request big tile mode */
