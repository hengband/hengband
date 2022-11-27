#pragma once

#include "dungeon/quest.h"
#include "system/angband.h"

class ItemEntity;
class PlayerType;
bool object_is_bounty(PlayerType *player_ptr, ItemEntity *o_ptr);
bool object_is_quest_target(QuestId quest_idx, ItemEntity *o_ptr);
