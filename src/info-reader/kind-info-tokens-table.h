#pragma once

#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include <string_view>
#include <unordered_map>

extern const std::unordered_map<std::string_view, tr_type> k_info_flags;
extern const std::unordered_map<std::string_view, ItemGenerationTraitType> k_info_gen_flags;
