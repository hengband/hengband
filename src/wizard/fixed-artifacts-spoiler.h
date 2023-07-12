#pragma once

#include <string>
#include <string_view>
#include <vector>

enum class SpoilerOutputResultType;
void spoiler_outlist(std::string_view header, const std::vector<std::string> &descriptions, char seperator);
SpoilerOutputResultType spoil_fixed_artifact();
