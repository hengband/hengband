#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class FileDisplayer {
public:
    FileDisplayer(std::string_view player_name)
        : player_name(player_name)
    {
    }

    void display(bool show_version, std::string_view name_with_tag, int initial_line, uint32_t mode, std::string_view what = "");

private:
    bool is_terminated = false;
    std::string player_name;
};
