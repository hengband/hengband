#pragma once

#include "system/angband.h"

struct player_type;
bool fire_ball(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_breath(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_rocket(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_ball_hide(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad);
bool fire_meteor(player_type *caster_ptr, MONSTER_IDX who, EFFECT_ID typ, POSITION x, POSITION y, HIT_POINT dam, POSITION rad);
bool fire_bolt(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
bool fire_blast(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, DICE_NUMBER dd, DICE_SID ds, int num, int dev);
bool fire_beam(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
bool fire_bolt_or_beam(player_type *caster_ptr, PERCENTAGE prob, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam);
bool project_hook(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, BIT_FLAGS flg);
