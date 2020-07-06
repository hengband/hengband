#pragma once

#include "system/angband.h"

void get_table_name_aux(char *out_string);
void get_table_name(char *out_string);
void get_table_sindarin_aux(char *out_string);
void get_table_sindarin(char *out_string);
void flavor_init(void);
char *object_desc_kosuu(char *t, object_type *o_ptr);
void object_desc(player_type *player_ptr, char *buf, object_type *o_ptr, BIT_FLAGS mode);
void strip_name(char *buf, KIND_OBJECT_IDX k_idx);
bool has_lite_flag(BIT_FLAGS *flags);
bool has_dark_flag(BIT_FLAGS *flags);
