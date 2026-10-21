#pragma once

#include "info-reader/parse-error-types.h"
#include <tl/expected.hpp>

class FloorType;
class PlayerType;
struct TownMapBuildingRule;
struct TownMapFeatureRule;

tl::expected<void, parse_error_type> apply_town_building_rule(const TownMapBuildingRule &rule);
tl::expected<void, parse_error_type> apply_town_map_feature(const FloorType &floor, const TownMapFeatureRule &feature);
void load_town_map(PlayerType *player_ptr);
