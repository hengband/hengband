#pragma once

#include "io/condition-expression.h"
#include <string>
#include <string_view>
#include <tl/optional.hpp>

class PlayerType;
tl::optional<std::string> resolve_common_expression_variable(PlayerType *player_ptr, std::string_view name);
std::string process_pref_file_expr(PlayerType *player_ptr, std::string_view expr);
tl::expected<std::string, ConditionExpressionError> process_pref_file_expr_checked(PlayerType *player_ptr, std::string_view expr);
