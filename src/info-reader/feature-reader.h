#pragma once

#include "system/angband.h"
#include <string_view>

struct angband_header;
errr parse_f_info(std::string_view buf, angband_header *head);
errr init_feat_variables(void);
FEAT_IDX f_tag_to_index(std::string_view str);
int16_t f_tag_to_index_in_init(concptr str);
void retouch_f_info(angband_header *head);
