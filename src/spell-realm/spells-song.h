#pragma once

#include "system/angband.h"

struct player_type;
void check_music(player_type *caster_ptr);
bool set_tim_stealth(player_type *creature_ptr, TIME_EFFECT v, bool do_dec);
void stop_singing(player_type *creature_ptr);
bool music_singing(player_type *caster_ptr, int music_songs);
bool music_singing_any(player_type *creature_ptr);
int32_t get_singing_song_effect(const player_type *creature_ptr);
void set_singing_song_effect(player_type *creature_ptr, const int32_t magic_num);
int32_t get_interrupting_song_effect(const player_type *creature_ptr);
void set_interrupting_song_effect(player_type *creature_ptr, const int32_t magic_num);
int32_t get_singing_count(const player_type *creature_ptr);
void set_singing_count(player_type *creature_ptr, const int32_t magic_num);
byte get_singing_song_id(const player_type *creature_ptr);
void set_singing_song_id(player_type *creature_ptr, const byte magic_num);
