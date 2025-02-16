#pragma once

#include <optional>

class MonsterEntity;
class PlayerType;
void sanity_blast(PlayerType *player_ptr, std::optional<short> m_idx = std::nullopt, bool necro = false);
