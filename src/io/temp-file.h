#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <tl/optional.hpp>

class TempFile {
public:
    TempFile();
    ~TempFile();

    const std::filesystem::path &get_path() const;
    void write_line(std::string_view line) const;
    tl::optional<std::string> read_line();
    void write_lines(const std::vector<std::string> &lines) const;
    std::vector<std::string> read_all();

private:
    std::filesystem::path path;
    size_t current_line_index;

    void create_empty_file() const;
    void set_permissions() const;
};
