#pragma once

#include <string_view>
#include <vector>

class DungeonFeeling {
public:
    ~DungeonFeeling() = default;
    DungeonFeeling(const DungeonFeeling &) = delete;
    DungeonFeeling(DungeonFeeling &&) = delete;
    DungeonFeeling &operator=(const DungeonFeeling &) = delete;
    DungeonFeeling &operator=(DungeonFeeling &&) = delete;
    static DungeonFeeling &get_instance();

    int get_feeling() const;
    void set_feeling(int new_feeling);
    std::string_view get_feeling_normal() const;
    std::string_view get_feeling_combat() const;
    std::string_view get_feeling_lucky() const;

private:
    DungeonFeeling() = default;
    static DungeonFeeling instance;

    int feeling = 0;
};
