#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

class PlayerType;
void player_immunity(PlayerType *player_ptr, TrFlags &flags);
void tim_player_immunity(PlayerType *player_ptr, TrFlags &flags);
void known_obj_immunity(PlayerType *player_ptr, TrFlags &flags);
void player_vulnerability_flags(PlayerType *player_ptr, TrFlags &flags);
