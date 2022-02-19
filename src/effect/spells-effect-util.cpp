#include "effect/spells-effect-util.h"

int rakubadam_m;
int rakubadam_p;
bool sukekaku;

int project_length = 0;

std::shared_ptr<CapturedMonsterType> g_cap_mon_ptr = std::make_shared<CapturedMonsterType>();

int project_m_n;
POSITION project_m_x;
POSITION project_m_y;
POSITION monster_target_x;
POSITION monster_target_y;
