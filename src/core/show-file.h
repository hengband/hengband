#pragma once

#include <cstdint>
#include <string_view>

class FileDisplayer {
public:
    FileDisplayer() = default;

    void display(std::string_view player_name, bool show_version, std::string_view name_with_tag, int initial_line, uint32_t mode, std::string_view what = "");

private:
    bool is_terminated = false;
};

void str_tolower(char *str);
