#pragma once

#include <functional>
#include <string_view>

enum class SpoilerOutputResultType;
class MonraceDefinition;
SpoilerOutputResultType spoil_mon_desc(std::string_view filename, std::function<bool(const MonraceDefinition &)> filter_monster = nullptr);
SpoilerOutputResultType spoil_mon_info();
