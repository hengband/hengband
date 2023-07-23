#pragma once

#include "dungeon/quest.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
bool object_is_bounty(PlayerType *player_ptr, const ItemEntity *o_ptr);
bool object_is_quest_target(QuestId quest_idx, const ItemEntity *o_ptr);
