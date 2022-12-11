#pragma once

#include "system/angband.h"

#include "object-enchant/tr-flags.h"

struct describe_option_type {
    BIT_FLAGS mode;
    bool aware;
    bool known; // 鑑定 or *鑑定* 済.
    bool flavor;
};

class ItemEntity;
char *get_ability_abbreviation(char *ptr, const ItemEntity *o_ptr, bool kanji, bool all);
void get_inscription(char *buff, const ItemEntity *o_ptr);
bool has_lite_flag(const TrFlags &flags);
bool has_dark_flag(const TrFlags &flags);

#ifdef JP
std::string describe_count_with_counter_suffix(const ItemEntity &item);
#endif
