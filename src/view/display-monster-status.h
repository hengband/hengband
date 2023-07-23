#pragma once

#include "system/angband.h"
#include <string>

class MonsterEntity;
std::string look_mon_desc(MonsterEntity *m_ptr, BIT_FLAGS mode);
