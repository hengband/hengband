#pragma once

#include <string>
#include <utility>

class ItemEntity;
class PlayerType;
struct describe_option_type;
std::pair<std::string, std::string> switch_tval_description(const PlayerType &player, const ItemEntity &item, const describe_option_type &opt);
