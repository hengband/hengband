#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

#include <functional>

class MonsterRaceInfo;
SpoilerOutputResultType spoil_mon_desc_all(concptr fname);
SpoilerOutputResultType spoil_mon_desc(concptr fname, std::function<bool(const MonsterRaceInfo *)> filter_monster = nullptr);
SpoilerOutputResultType spoil_mon_info(concptr fname);
