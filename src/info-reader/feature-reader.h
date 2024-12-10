#pragma once

#include "system/angband.h"
#include <string_view>

struct angband_header;
errr parse_terrains_info(std::string_view buf, angband_header *head);
void init_feat_variables();
