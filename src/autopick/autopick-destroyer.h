#pragma once

typedef struct object_type object_type;
typedef struct player_type player_type;
void auto_destroy_item(player_type *player_ptr, object_type *o_ptr, int autopick_idx);
