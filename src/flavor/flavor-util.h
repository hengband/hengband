#pragma once

#include "system/angband.h"

struct describe_option_type {
    BIT_FLAGS mode;
    bool aware;
    bool known; // 鑑定 or *鑑定* 済.
    bool flavor;
};

class ItemEntity;
char *get_ability_abbreviation(char *ptr, const ItemEntity *o_ptr, bool kanji, bool all);
void get_inscription(char *buff, const ItemEntity *o_ptr);

#ifdef JP
std::string describe_count_with_counter_suffix(const ItemEntity &item);
#endif
