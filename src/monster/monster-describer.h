#pragma once

#include "system/angband.h"

class MonsterEntity;
class PlayerType;
void monster_desc(PlayerType *player_ptr, char *desc, MonsterEntity *m_ptr, BIT_FLAGS mode);
void message_pain(PlayerType *player_ptr, MONSTER_IDX m_idx, int dam);
