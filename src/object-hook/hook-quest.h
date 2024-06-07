#pragma once

enum class QuestId : short;
class ItemEntity;
bool object_is_bounty(const ItemEntity *o_ptr);
bool object_is_quest_target(QuestId quest_idx, const ItemEntity *o_ptr);
