#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
bool object_is_favorite(player_type *player_ptr, const object_type *o_ptr);
bool object_allow_two_hands_wielding(const object_type *o_ptr);
