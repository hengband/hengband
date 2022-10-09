#pragma once

#include "system/angband.h"
#include <string_view>

struct angband_header;
errr parse_baseitems_info(std::string_view buf, angband_header *head);
