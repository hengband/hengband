#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

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

    tl::expected<void, ParseErrorType> read();
    const std::optional<VaultReadError> &error() const;

private:
    tl::expected<void, ParseErrorType> fail(ParseErrorType code, std::string_view path, std::string_view reason);
    tl::expected<void, ParseErrorType> read_integer(std::string_view key, int &value, int minimum, int maximum);
    const nlohmann::json &data;
    std::optional<VaultReadError> diagnostic;
};
