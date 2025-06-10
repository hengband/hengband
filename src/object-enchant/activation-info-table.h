#pragma once

#include <string>
#include <tl/optional.hpp>
#include <vector>

enum class RandomArtActType : short;
class ActivationType {
public:
    std::string flag;
    RandomArtActType index;
    int level;
    int value;
    tl::optional<int> constant; // 発動間隔の最低ターン数
    int dice; // 発動間隔の追加ターン数
    std::string desc;

    tl::optional<std::string> build_timeout_description() const;
};

extern const std::vector<ActivationType> activation_info;
