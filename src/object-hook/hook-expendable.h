#pragma once

class ObjectType;
class PlayerType;
bool item_tester_hook_eatable(PlayerType *player_ptr, const ObjectType *o_ptr);
bool item_tester_hook_quaff(PlayerType *player_ptr, const ObjectType *o_ptr);
bool can_player_destroy_object(PlayerType *player_ptr, ObjectType *o_ptr);
