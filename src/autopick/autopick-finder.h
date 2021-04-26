#pragma once

#include "system/angband.h"

typedef struct object_type object_type;
typedef struct player_type player_type;
typedef struct text_body_type text_body_type;
int find_autopick_list(player_type *player_ptr, object_type *o_ptr);
bool get_object_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp);
bool get_destroyed_object_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp);
byte get_string_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp);
void search_for_object(player_type *player_ptr, text_body_type *tb, object_type *o_ptr, bool forward);
void search_for_string(text_body_type *tb, concptr search_str, bool forward);
