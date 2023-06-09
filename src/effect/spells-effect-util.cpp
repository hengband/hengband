#include "effect/spells-effect-util.h"
#include "monster-race/race-indice-types.h"

int rakubadam_m;
int rakubadam_p;
bool sukekaku;

int project_length = 0;

int project_m_n;
POSITION project_m_x;
POSITION project_m_y;
POSITION monster_target_x;
POSITION monster_target_y;

CapturedMonsterType::CapturedMonsterType()
    : r_idx(MonsterRaceId::PLAYER)
{
}
