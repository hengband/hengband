#pragma once

#include "system/angband.h"
#include "effect/attribute-types.h"

class PlayerType;
bool fire_ball(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad);
bool fire_breath(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad);
bool fire_rocket(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad);
bool fire_ball_hide(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, POSITION rad);
bool fire_meteor(PlayerType *player_ptr, MONSTER_IDX who, AttributeType typ, POSITION x, POSITION y, int dam, POSITION rad);
bool fire_bolt(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam);
bool fire_blast(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, DICE_NUMBER dd, DICE_SID ds, int num, int dev);
bool fire_beam(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam);
bool fire_bolt_or_beam(PlayerType *player_ptr, PERCENTAGE prob, AttributeType typ, DIRECTION dir, int dam);
bool project_hook(PlayerType *player_ptr, AttributeType typ, DIRECTION dir, int dam, BIT_FLAGS flg);
