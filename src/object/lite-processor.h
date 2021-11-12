#pragma once

struct object_type;
class PlayerType;
void reduce_lite_life(PlayerType *player_ptr);
void notice_lite_change(PlayerType* player_ptr, object_type* o_ptr);
