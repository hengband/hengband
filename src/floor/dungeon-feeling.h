#pragma once

#include <string_view>
#include <vector>

class DungeonFeeling {
public:
    DungeonFeeling() = default;
    std::string_view get_feeling_normal(int feeling) const;
    std::string_view get_feeling_combat(int feeling) const;
    std::string_view get_feeling_lucky(int feeling) const;
};
