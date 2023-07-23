#pragma once

#include <string>

class PlayerType;
enum class MindKindType;

std::string mindcraft_info(PlayerType *player_ptr, MindKindType use_mind, int power);
