#pragma once

#include "external-lib/include-json.h"
#include "system/angband.h"
#include <string_view>

struct angband_header;
errr parse_spell_info(nlohmann::json &spell_data, angband_header *head);
