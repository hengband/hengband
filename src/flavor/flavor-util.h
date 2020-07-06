#pragma once

#include "system/angband.h"

char *object_desc_chr(char *t, char c);
char *object_desc_str(char *t, concptr s);
char *object_desc_num(char *t, uint n);
char *object_desc_int(char *t, int v);
char *get_ability_abbreviation(player_type *player_ptr, char *ptr, object_type *o_ptr, bool kanji, bool all);
void get_inscription(player_type *player_ptr, char *buff, object_type *o_ptr);

#ifdef JP
char *object_desc_count_japanese(char *t, object_type *o_ptr);
#endif
