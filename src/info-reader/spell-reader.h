#pragma once

#include "external-lib/include-json.h"
#include "system/angband.h"
#include <vector>

class SpellInfo;
errr parse_spell_info(nlohmann::json &spell_data, std::vector<std::vector<SpellInfo>> &spell_list);
