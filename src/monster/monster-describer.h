#pragma once

#include "system/angband.h"
#include <string>

class MonsterEntity;
class PlayerType;
std::string monster_desc(PlayerType *player_ptr, const MonsterEntity *m_ptr, BIT_FLAGS mode);
