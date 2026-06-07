#pragma once

#include "system/angband.h"
#include <external-lib/include-json.h>

class SpellInfoList;

errr parse_spell_info(nlohmann::json &spell_data, SpellInfoList &spell_info_list);
