#pragma once

#include "system/angband.h"
#include <string>

class MonsterEntity;
class PlayerType;
std::string monster_desc(PlayerType *player_ptr, MonsterEntity *m_ptr, BIT_FLAGS mode);
void message_pain(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam);
