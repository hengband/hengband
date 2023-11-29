#pragma once

#include <string>
#include <vector>

enum class RandomArtActType : short;
struct activation_type {
    std::string flag;
    RandomArtActType index;
    int level;
    int value;
    struct {
        int constant;
        int dice;
    } timeout;
    std::string desc;
};

extern const std::vector<activation_type> activation_info;
