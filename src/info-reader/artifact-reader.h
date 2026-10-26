#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <tl/expected.hpp>

class ArtifactDefinition;

class ArtifactReader {
public:
    explicit ArtifactReader(const nlohmann::json &art_data);
    ArtifactReader(nlohmann::json &&) = delete;
    ArtifactReader(const ArtifactReader &) = delete;
    ArtifactReader(ArtifactReader &&) = delete;
    ArtifactReader &operator=(const ArtifactReader &) = delete;
    ArtifactReader &operator=(ArtifactReader &&) = delete;

    tl::expected<void, ParseErrorType> read() const;

private:
    bool grab_one_artifact_flag(ArtifactDefinition &artifact, std::string_view what) const;
    tl::expected<void, ParseErrorType> set_art_baseitem(ArtifactDefinition &artifact) const;
    tl::expected<void, ParseErrorType> set_art_activate(ArtifactDefinition &artifact) const;
    tl::expected<void, ParseErrorType> set_art_flags(ArtifactDefinition &artifact) const;

    const nlohmann::json &art_data;
};
