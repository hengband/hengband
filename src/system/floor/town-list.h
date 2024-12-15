#pragma once

#include <vector>

constexpr short VALID_TOWNS = 6; // @details 旧海底都市クエストのマップを除外する. 有効な町に差し替え完了したら不要になるので注意.
constexpr auto SECRET_TOWN = 5; // @details ズルの町番号.

class TownInfo;
extern std::vector<TownInfo> towns_info;
