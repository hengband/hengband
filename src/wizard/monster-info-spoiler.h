#pragma once

#include "wizard/spoiler-util.h"
#include <functional>
#include <string_view>

class MonsterRaceInfo;
SpoilerOutputResultType spoil_mon_desc(std::string_view filename, std::function<bool(const MonsterRaceInfo *)> filter_monster = nullptr);
SpoilerOutputResultType spoil_mon_info();
