#pragma once

#include <optional>
#include <string>
#include <vector>

enum class RandomArtActType : short;
class ActivationType {
public:
    std::string flag;
    RandomArtActType index;
    int level;
    int value;
    std::optional<int> constant; // 発動間隔の最低ターン数
    int dice; // 発動間隔の追加ターン数
    std::string desc;
};

extern const std::vector<ActivationType> activation_info;
