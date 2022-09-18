#pragma once

#include "system/angband.h"
#include <string>

void get_table_name_aux(char *out_string);
void get_table_name(char *out_string);
void get_table_sindarin_aux(char *out_string);
void get_table_sindarin(char *out_string);
void flavor_init(void);
std::string strip_name(KIND_OBJECT_IDX k_idx);
