#pragma once

#include "room/door-definition.h"
#include "util/point-2d.h"

class PlayerType;
void add_door(PlayerType *player_ptr, const Pos2D &pos);
void place_secret_door(PlayerType *player_ptr, const Pos2D &pos, DoorKind door_kind = DoorKind::DEFAULT);
void place_locked_door(PlayerType *player_ptr, const Pos2D &pos);
void place_random_door(PlayerType *player_ptr, const Pos2D &pos, bool is_room_door);
void place_closed_door(PlayerType *player_ptr, const Pos2D &pos, DoorKind door_kind);
