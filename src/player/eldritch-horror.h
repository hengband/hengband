#pragma once

#include <tl/optional.hpp>

class MonsterEntity;
class PlayerType;
void sanity_blast(PlayerType *player_ptr, tl::optional<short> m_idx = tl::nullopt, bool necro = false);
