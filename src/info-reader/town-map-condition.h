#pragma once

#include "io/condition-expression.h"
#include <optional>
#include <string>
#include <string_view>

class PlayerType;
tl::expected<bool, ConditionExpressionFailure> is_town_map_condition_met(PlayerType *player_ptr, const std::optional<std::string> &condition);
std::string format_town_map_condition_error(std::string_view filename, std::string_view location, std::string_view expression, std::string_view variable);
