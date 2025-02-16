#pragma once

#include <string>

extern std::string screen_dump;

#ifdef WORLD_SCORE

class PlayerType;
bool report_score(PlayerType *player_ptr);
std::string make_screen_dump(PlayerType *player_ptr);
#endif
