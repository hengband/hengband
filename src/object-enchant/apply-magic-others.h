#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
void apply_magic_others(player_type *owner_ptr, object_type *o_ptr, int power);
