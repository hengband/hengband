#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
int home_carry(player_type *player_ptr, object_type *o_ptr);
bool combine_and_reorder_home(player_type *player_ptr, const int store_num);
