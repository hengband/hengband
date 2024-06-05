#pragma once

#include "system/angband.h"
#include <string_view>

struct angband_header;
errr parse_terrains_info(std::string_view buf, angband_header *head);
errr init_feat_variables();
short f_tag_to_index(std::string_view tag);
short f_tag_to_index_in_init(std::string_view tag);
void retouch_terrains_info();
