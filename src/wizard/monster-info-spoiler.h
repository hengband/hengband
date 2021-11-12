#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

#include <functional>

struct monster_race;
SpoilerOutputResultType spoil_mon_desc_all(concptr fname);
SpoilerOutputResultType spoil_mon_desc(concptr fname, std::function<bool(const monster_race *)> filter_monster = nullptr);
SpoilerOutputResultType spoil_mon_info(concptr fname);
