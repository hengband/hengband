#pragma once

#include "system/angband.h"

class MonsterAttackPlayer;
class PlayerType;
void check_fall_off_horse(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr);
bool process_fall_off_horse(PlayerType *player_ptr, HIT_POINT dam, bool force);
