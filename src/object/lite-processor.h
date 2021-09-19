#pragma once

struct object_type;;
struct player_type;
void reduce_lite_life(player_type *player_ptr);
void notice_lite_change(player_type* player_ptr, object_type* o_ptr);
