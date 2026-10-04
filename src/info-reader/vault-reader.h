#pragma once

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>

struct VaultReadError {
    std::string id;
    std::string path;
    std::string reason;
};

class VaultReader {
public:
    explicit VaultReader(const nlohmann::json &data);
    VaultReader(nlohmann::json &&) = delete;
    VaultReader(const VaultReader &) = delete;
    VaultReader(VaultReader &&) = delete;
    VaultReader &operator=(const VaultReader &) = delete;
    VaultReader &operator=(VaultReader &&) = delete;

    int read();
    const std::optional<VaultReadError> &error() const;

private:
    int fail(int code, std::string_view path, std::string_view reason);
    int read_integer(std::string_view key, int &value, int minimum, int maximum);
    const nlohmann::json &data;
    std::optional<VaultReadError> diagnostic;
};
