#pragma once

class ItemEntity;
class PlayerType;
void reduce_lite_life(PlayerType *player_ptr);
void notice_lite_change(PlayerType *player_ptr, ItemEntity *o_ptr);
