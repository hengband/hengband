#pragma once

#include "system/angband.h"

class PlayerType;
bool fire_ball(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_breath(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_rocket(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_ball_hide(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_meteor(PlayerType *player_ptr, MONSTER_IDX who, EFFECT_ID typ, POSITION x, POSITION y, HIT_POINT dam, POSITION rad);
bool fire_bolt(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
bool fire_blast(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, DICE_NUMBER dd, DICE_SID ds, int num, int dev);
bool fire_beam(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
bool fire_bolt_or_beam(PlayerType *player_ptr, PERCENTAGE prob, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
bool project_hook(PlayerType *player_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, BIT_FLAGS flg);
