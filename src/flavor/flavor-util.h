#pragma once

#include "system/angband.h"
#include "system/system-variables.h"

#include "object-enchant/tr-flags.h"

class BaseItemInfo;
class ObjectType;
struct flavor_type {
    char *buf;
    ObjectType *o_ptr;
    BIT_FLAGS mode;
    concptr kindname;
    concptr basenm;
    concptr modstr;
    int power;
    int fire_rate;
    bool aware;
    bool known; // 鑑定 or *鑑定* 済.
    bool flavor;
    bool show_weapon;
    bool show_armour;
    concptr s;
    concptr s0;
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
    ObjectType *bow_ptr;
    BaseItemInfo *k_ptr;
    BaseItemInfo *flavor_k_ptr;
    int avgdam;
};

class PlayerType;
flavor_type *initialize_flavor_type(flavor_type *flavor_ptr, char *buf, ObjectType *o_ptr, BIT_FLAGS mode);
char *object_desc_chr(char *t, char c);
char *object_desc_str(char *t, concptr s);
char *object_desc_num(char *t, uint n);
char *object_desc_int(char *t, int v);
char *get_ability_abbreviation(char *ptr, ObjectType *o_ptr, bool kanji, bool all);
void get_inscription(char *buff, ObjectType *o_ptr);
bool has_lite_flag(const TrFlags &flags);
bool has_dark_flag(const TrFlags &flags);

#ifdef JP
char *object_desc_count_japanese(char *t, ObjectType *o_ptr);
#endif
