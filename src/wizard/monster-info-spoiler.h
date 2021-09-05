#pragma once

#include "system/angband.h"
#include "wizard/spoiler-util.h"

#include <functional>

struct monster_race;
spoiler_output_status spoil_mon_desc_all(concptr fname);
spoiler_output_status spoil_mon_desc(concptr fname, std::function<bool(const monster_race *)> filter_monster = nullptr);
spoiler_output_status spoil_mon_info(concptr fname);
