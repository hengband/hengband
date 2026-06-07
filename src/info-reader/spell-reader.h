#pragma once

#include "system/angband.h"
#include <nlohmann/json.hpp>

class SpellInfoList;

errr parse_spell_info(nlohmann::json &spell_data, SpellInfoList &spell_info_list);
