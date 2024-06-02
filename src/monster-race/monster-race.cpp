#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-indice-types.h"
#include "system/monster-race-info.h"
#include "world/world.h"
#include <algorithm>
#include <vector>

/* The monster race arrays */
std::map<MonsterRaceId, MonsterRaceInfo> monraces_info;

MonsterRace::MonsterRace(MonsterRaceId r_idx)
    : r_idx(r_idx)
{
}
