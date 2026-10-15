#pragma once

class PlayerType;
void stop_running(PlayerType *player_ptr);
void disturb(PlayerType *player_ptr, bool stop_search, bool stop_travel);
