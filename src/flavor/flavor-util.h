#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

#include "object-enchant/tr-flags.h"

class BaseitemInfo;
class ItemEntity;
struct flavor_type {
    char *buf;
    ItemEntity *o_ptr;
    BIT_FLAGS mode;
    int power;
    int fire_rate;
    bool aware;
    bool known; // 鑑定 or *鑑定* 済.
    bool flavor;
    char *t;
    char p1; // const.
    char p2; // const.
    char b1; // const.
    char b2; // const.
    char c1; // const.
    char c2; // const.
    char tmp_val[MAX_NLEN + 160];
    char tmp_val2[MAX_NLEN + 10];
    char fake_insc_buf[30];
    TrFlags tr_flags;
    ItemEntity *bow_ptr;
    BaseitemInfo *k_ptr;
    BaseitemInfo *flavor_k_ptr;
    int avgdam;
};

struct describe_option_type {
    BIT_FLAGS mode;
    bool aware;
    bool known; // 鑑定 or *鑑定* 済.
    bool flavor;
};

class PlayerType;
flavor_type *initialize_flavor_type(flavor_type *flavor_ptr, char *buf, ItemEntity *o_ptr, BIT_FLAGS mode);
char *object_desc_chr(char *t, char c);
char *object_desc_str(char *t, concptr s);
char *object_desc_num(char *t, uint n);
char *object_desc_int(char *t, int v);
char *get_ability_abbreviation(char *ptr, ItemEntity *o_ptr, bool kanji, bool all);
void get_inscription(char *buff, ItemEntity *o_ptr);
bool has_lite_flag(const TrFlags &flags);
bool has_dark_flag(const TrFlags &flags);

#ifdef JP
char *object_desc_count_japanese(char *t, const ItemEntity *o_ptr);
#endif
