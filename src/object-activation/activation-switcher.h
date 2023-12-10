#pragma once

#include <string_view>

enum class RandomArtActType : short;
class ItemEntity;
class PlayerType;
bool switch_activation(PlayerType *player_ptr, ItemEntity **o_ptr_ptr, const RandomArtActType index, std::string_view name);
