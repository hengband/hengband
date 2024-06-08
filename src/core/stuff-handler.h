#pragma once

class PlayerType;
void handle_stuff(PlayerType *player_ptr);
void object_kind_track(PlayerType *player_ptr, short bi_id);
void health_track(PlayerType *player_ptr, short m_idx);
bool update_player();
bool redraw_player(PlayerType *player_ptr);
