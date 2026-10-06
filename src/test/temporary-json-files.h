#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace test {

/*!
 * @brief JSONC 読み込みテスト用のファイルだけを管理する。グローバル状態の復元は呼び出し側で行う。
 */
class TemporaryJsonFiles {
public:
    TemporaryJsonFiles()
        : directory(create_directory())
    {
    }

    TemporaryJsonFiles(const TemporaryJsonFiles &) = delete;
    TemporaryJsonFiles &operator=(const TemporaryJsonFiles &) = delete;
    TemporaryJsonFiles(TemporaryJsonFiles &&) = delete;
    TemporaryJsonFiles &operator=(TemporaryJsonFiles &&) = delete;

    ~TemporaryJsonFiles()
    {
        std::error_code error;
        std::filesystem::remove_all(this->directory, error);
    }

    void write(const std::filesystem::path &name, const nlohmann::json &data) const
    {
        this->write_raw(name, data.dump());
    }

    void write_raw(const std::filesystem::path &name, std::string_view data) const
    {
        if (name.empty() || name.has_root_path() || name.filename().empty() || name.filename() == ".") {
            throw std::invalid_argument("Temporary test file name must be relative");
        }
        for (const auto &component : name) {
            if (component == "..") {
                throw std::invalid_argument("Temporary test file name must not escape its directory");
            }
        }

        const auto path = this->directory / name;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output;
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.open(path, std::ios::binary | std::ios::trunc);
        output << data;
        output.close();
    }

    const std::filesystem::path directory;

private:
    static std::filesystem::path create_directory()
    {
        static std::atomic<unsigned long long> sequence{ 0 };
        const auto parent = std::filesystem::temp_directory_path();
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (auto attempt = 0; attempt < 100; ++attempt) {
            const auto serial = sequence.fetch_add(1, std::memory_order_relaxed);
            auto candidate = parent / ("hengband-json-test-" + std::to_string(stamp) + "-" + std::to_string(serial));
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error)) {
                return candidate;
            }
            if (error && error != std::errc::file_exists) {
                throw std::filesystem::filesystem_error("Cannot create isolated JSON test directory", candidate, error);
            }
        }
        throw std::runtime_error("Cannot create isolated JSON test directory");
    }
};

}
