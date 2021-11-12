#pragma once

struct object_type;
class PlayerType;
void auto_destroy_item(PlayerType *player_ptr, object_type *o_ptr, int autopick_idx);
