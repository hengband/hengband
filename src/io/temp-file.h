#pragma once

#include <filesystem>
#include <string>
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
    const tl::optional<std::string> &get_error_message() const;
    std::vector<std::string> read_all();
    void write_lines(const std::vector<std::string> &lines);

private:
    std::filesystem::path path;
    tl::optional<std::string> error_message;
};
