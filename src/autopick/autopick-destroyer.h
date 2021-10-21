#pragma once

struct object_type;;
class player_type;
void auto_destroy_item(player_type *player_ptr, object_type *o_ptr, int autopick_idx);
