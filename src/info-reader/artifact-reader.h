#pragma once

#include "external-lib/include-json.h"
#include "system/angband.h"
#include <string_view>

struct angband_header;
errr parse_artifacts_info(nlohmann::json &element, angband_header *head);
