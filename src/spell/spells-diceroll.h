#pragma once

#include "system/angband.h"

struct monster_type;
class PlayerType;
bool common_saving_throw_control(PlayerType *player_ptr, int pow, monster_type *m_ptr);
bool common_saving_throw_charm(PlayerType *player_ptr, int pow, monster_type *m_ptr);
PERCENTAGE beam_chance(PlayerType *player_ptr);
