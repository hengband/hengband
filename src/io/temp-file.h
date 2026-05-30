#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

class TempFile {
public:
    TempFile();
    TempFile(const TempFile &) = delete;
    TempFile(TempFile &&) = delete;
    TempFile &operator=(const TempFile &) = delete;
    TempFile &operator=(TempFile &&) = delete;
    ~TempFile();

    const std::filesystem::path &get_path() const;
    void write_line(std::string_view line) const;
    tl::optional<std::string> read_line();
    void write_lines(const std::vector<std::string> &lines) const;
    std::vector<std::string> read_all();

private:
    std::filesystem::path path;
    std::ifstream ifs;
};
