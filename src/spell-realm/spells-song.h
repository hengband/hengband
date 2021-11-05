#pragma once

#include "system/angband.h"

class PlayerType;
void check_music(PlayerType *player_ptr);
bool set_tim_stealth(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec);
void stop_singing(PlayerType *player_ptr);
bool music_singing(PlayerType *player_ptr, int music_songs);
bool music_singing_any(PlayerType *player_ptr);
int32_t get_singing_song_effect(PlayerType *player_ptr);
void set_singing_song_effect(PlayerType *player_ptr, const int32_t magic_num);
int32_t get_interrupting_song_effect(PlayerType *player_ptr);
void set_interrupting_song_effect(PlayerType *player_ptr, const int32_t magic_num);
int32_t get_singing_count(PlayerType *player_ptr);
void set_singing_count(PlayerType *player_ptr, const int32_t magic_num);
byte get_singing_song_id(PlayerType *player_ptr);
void set_singing_song_id(PlayerType *player_ptr, const byte magic_num);
