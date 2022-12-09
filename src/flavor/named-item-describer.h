#pragma once

#include <string>

struct describe_option_type;
class PlayerType;
class ItemEntity;
std::string describe_named_item(PlayerType *player_ptr, const ItemEntity &item, const describe_option_type &opt);
