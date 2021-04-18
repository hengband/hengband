#pragma once

#include "system/angband.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include <string>
#include <unordered_map>

extern std::unordered_map<std::string_view, tr_type> k_info_flags;
extern std::unordered_map<std::string_view, TRG> k_info_gen_flags;
