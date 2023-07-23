#pragma once

class ItemEntity;
class PlayerType;
void object_known(ItemEntity *o_ptr);
void object_aware(PlayerType *player_ptr, const ItemEntity *o_ptr);
void object_tried(const ItemEntity *o_ptr);
