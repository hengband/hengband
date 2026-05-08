#pragma once

#include "util/abstract-map-wrapper.h"
#include <string>

enum class FixedArtifactId : short;
class ArtifactDefinition;
class ItemEntity;
class ArtifactList : public util::AbstractMapWrapper<FixedArtifactId, ArtifactDefinition> {
public:
    ArtifactList(const ArtifactList &) = delete;
    ArtifactList(ArtifactList &&) = delete;
    ArtifactList &operator=(const ArtifactList &) = delete;
    ArtifactList &operator=(ArtifactList &&) = delete;
    ~ArtifactList() = default;

    static ArtifactList &get_instance();
    const ArtifactDefinition &get_artifact(const FixedArtifactId fa_id) const;

    bool order(const FixedArtifactId id1, const FixedArtifactId id2) const;
    void emplace(const FixedArtifactId fa_id, ArtifactDefinition &&artifact);
    std::string get_full_name(const FixedArtifactId fa_id) const;

private:
    ArtifactList() = default;
    static ArtifactList instance;
    static ArtifactDefinition dummy;

    std::map<FixedArtifactId, ArtifactDefinition> artifacts{};

    std::map<FixedArtifactId, ArtifactDefinition> &get_inner_container() override
    {
        return this->artifacts;
    }

    void validate_fa_id(const FixedArtifactId fa_id) const;
};
