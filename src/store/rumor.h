#pragma once

#include <tl/optional.hpp>

enum class RumorRarity;
void display_random_rumor(tl::optional<RumorRarity> rarity = tl::nullopt);
