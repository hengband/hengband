#pragma once

#include <string>
#include <vector>

enum class RandomArtActType : short;
struct activation_type {
    std::string flag;
    RandomArtActType index;
    int level;
    int value;
    int constant; // 発動間隔の最低ターン数
    int dice; // 発動間隔の追加ターン数
    std::string desc;
};

extern const std::vector<activation_type> activation_info;
