#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

enum class SpoilerOutputResultType;
void spoiler_outlist(std::string_view header, const std::vector<std::string> &descriptions, char seperator, std::ofstream &ofs);
SpoilerOutputResultType spoil_fixed_artifact();
