#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
void get_bloody_moon_flags(object_type *o_ptr);
bool activate_bloody_moon(player_type *user_ptr, object_type *o_ptr);
