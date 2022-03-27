#pragma once

class ObjectType;
class PlayerType;
void object_known(ObjectType *o_ptr);
void object_aware(PlayerType *player_ptr, const ObjectType *o_ptr);
void object_tried(const ObjectType *o_ptr);
