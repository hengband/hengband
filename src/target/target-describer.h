#pragma once

#include "system/angband.h"
#include <string>

extern bool show_gold_on_floor;

enum target_type : uint32_t;
class MonsterEntity;
class PlayerType;
std::string evaluate_monster_exp(PlayerType *player_ptr, const MonsterEntity &monster);
char examine_grid(PlayerType *player_ptr, const POSITION y, const POSITION x, target_type mode, concptr info);
