#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include <optional>

class CapturedMonsterType;
class Dice;
class Direction;
class PlayerType;
bool fire_ball(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad, std::optional<CapturedMonsterType *> cap_mon_ptr = std::nullopt);
bool fire_breath(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad);
bool fire_rocket(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad);
bool fire_ball_hide(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad);
bool fire_meteor(PlayerType *player_ptr, MONSTER_IDX src_idx, AttributeType typ, POSITION x, POSITION y, int dam, POSITION rad);
bool fire_bolt(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam);
bool fire_bolt(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam);
bool fire_blast(PlayerType *player_ptr, AttributeType typ, const Direction &dir, const Dice &dice, int num, int dev);
bool fire_beam(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam);
bool fire_bolt_or_beam(PlayerType *player_ptr, PERCENTAGE prob, AttributeType typ, const Direction &dir, int dam);
bool project_hook(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, BIT_FLAGS flg);
