#pragma once

#include <cstdint>
#include <optional>

enum class MonraceId : short;
class FloorType;
class MonraceDefinition;
class PlayerType;
MonraceId get_mon_num(PlayerType *player_ptr, int min_level, int max_level, uint32_t mode);
void choose_chameleon_polymorph(PlayerType *player_ptr, short m_idx, short terrain_id, std::optional<short> summoner_m_idx = std::nullopt);
int get_monster_crowd_number(const FloorType *floor_ptr, short m_idx);
