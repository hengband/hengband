#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include <tl/optional.hpp>

class CapturedMonsterType;
class Dice;
class Direction;
class PlayerType;
class AbstractAttribute;
bool fire_ball(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad, std::shared_ptr<AbstractAttribute> attribute = nullptr, tl::optional<CapturedMonsterType *> cap_mon_ptr = tl::nullopt);
bool fire_breath(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool fire_rocket(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool fire_ball_hide(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, POSITION rad, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool fire_meteor(PlayerType *player_ptr, MONSTER_IDX src_idx, AttributeType typ, POSITION x, POSITION y, int dam, POSITION rad, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool fire_bolt(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool fire_blast(PlayerType *player_ptr, AttributeType typ, const Direction &dir, const Dice &dice, int num, int dev, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool fire_beam(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool fire_bolt_or_beam(PlayerType *player_ptr, PERCENTAGE prob, AttributeType typ, const Direction &dir, int dam, std::shared_ptr<AbstractAttribute> attribute = nullptr);
bool project_hook(PlayerType *player_ptr, AttributeType typ, const Direction &dir, int dam, BIT_FLAGS flg, std::shared_ptr<AbstractAttribute> attribute = nullptr);
