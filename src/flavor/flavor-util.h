#pragma once

#include "system/angband.h"

struct describe_option_type {
    BIT_FLAGS mode;
    bool aware;
    bool known; // 鑑定 or *鑑定* 済.
    bool flavor;
};

class ItemEntity;
std::string get_ability_abbreviation(const ItemEntity &o_ptr, bool is_kanji, bool all);
std::string get_inscription(const ItemEntity &item);

#ifdef JP
std::string describe_count_with_counter_suffix(const ItemEntity &item);
#endif
