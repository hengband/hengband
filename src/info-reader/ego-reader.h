#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <tl/expected.hpp>

class EgoItemDefinition;
struct ego_generate_type;

class EgoReader {
public:
    explicit EgoReader(const nlohmann::json &ego_data);
    EgoReader(nlohmann::json &&) = delete;
    EgoReader(const EgoReader &) = delete;
    EgoReader(EgoReader &&) = delete;
    EgoReader &operator=(const EgoReader &) = delete;
    EgoReader &operator=(EgoReader &&) = delete;

    static constexpr int FORMAT_VERSION = 2;
    static tl::expected<void, ParseErrorType> validate_root(const nlohmann::json &root);
    tl::expected<void, ParseErrorType> read() const;

private:
    bool grab_one_flag(EgoItemDefinition &ego, const nlohmann::json &flag) const;
    bool grab_one_extra_flag(ego_generate_type &extra, const nlohmann::json &flag) const;
    tl::expected<void, ParseErrorType> set_flags(EgoItemDefinition &ego) const;
    tl::expected<void, ParseErrorType> set_extra_flags(EgoItemDefinition &ego) const;
    tl::expected<void, ParseErrorType> set_activation(EgoItemDefinition &ego) const;

    const nlohmann::json &ego_data;
};
