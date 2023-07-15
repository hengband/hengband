#pragma once

#include "system/angband.h"

#if !defined(DISABLE_NET)

#include <string_view>

bool report_error(std::string_view description);

#endif // !defined(DISABLE_NET)
