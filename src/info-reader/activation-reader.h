#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <tl/expected.hpp>

enum class RandomArtActType : short;

RandomArtActType grab_one_activation_flag(std::string_view what);
tl::expected<RandomArtActType, ParseErrorType> read_activation(const nlohmann::json &activation_json);
