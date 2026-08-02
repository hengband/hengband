#pragma once

#include <nlohmann/json.hpp>
#include <string_view>

class ArtifactDefinition;

class ArtifactReader {
public:
    explicit ArtifactReader(const nlohmann::json &art_data);
    ArtifactReader(nlohmann::json &&) = delete;
    ArtifactReader(const ArtifactReader &) = delete;
    ArtifactReader(ArtifactReader &&) = delete;
    ArtifactReader &operator=(const ArtifactReader &) = delete;
    ArtifactReader &operator=(ArtifactReader &&) = delete;

    int read() const;

private:
    bool grab_one_artifact_flag(ArtifactDefinition &artifact, std::string_view what) const;
    int set_art_baseitem(ArtifactDefinition &artifact) const;
    int set_art_activate(ArtifactDefinition &artifact) const;
    int set_art_flags(ArtifactDefinition &artifact) const;

    const nlohmann::json &art_data;
};
