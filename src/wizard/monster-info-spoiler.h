#pragma once

#include <functional>
#include <string_view>

enum class SpoilerOutputResultType;
class MonsterRaceInfo;
SpoilerOutputResultType spoil_mon_desc(std::string_view filename, std::function<bool(const MonsterRaceInfo *)> filter_monster = nullptr);
SpoilerOutputResultType spoil_mon_info();
