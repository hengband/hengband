#pragma once

#include "system/angband.h"

struct object_type;
class PlayerType;
struct text_body_type;
int find_autopick_list(PlayerType *player_ptr, object_type *o_ptr);
bool get_object_for_search(PlayerType *player_ptr, object_type **o_handle, concptr *search_strp);
bool get_destroyed_object_for_search(PlayerType *player_ptr, object_type **o_handle, concptr *search_strp);
byte get_string_for_search(PlayerType *player_ptr, object_type **o_handle, concptr *search_strp);
void search_for_object(PlayerType *player_ptr, text_body_type *tb, object_type *o_ptr, bool forward);
void search_for_string(text_body_type *tb, concptr search_str, bool forward);
