#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

class TempFile {
public:
    TempFile();
    TempFile(const std::filesystem::path &path);
    TempFile(const TempFile &) = delete;
    TempFile(TempFile &&) = delete;
    TempFile &operator=(const TempFile &) = delete;
    TempFile &operator=(TempFile &&) = delete;
    ~TempFile();

    const std::filesystem::path &get_path() const;
    std::vector<std::string> read_all() const;
    void write_lines(const std::vector<std::string> &lines) const;

private:
    std::filesystem::path path;
};
