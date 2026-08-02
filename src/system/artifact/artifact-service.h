#pragma once

#include <memory>
#include <tl/optional.hpp>

enum class FixedArtifactId : short;
class ArtifactDefinition;
class BaseitemKey;
class ItemEntity;
class ArtifactService {
public:
    ArtifactService() = delete;

    static tl::optional<FixedArtifactId> find_generatable_fixed_artifact(const BaseitemKey &bi_key, int dungeon_level);
    static tl::optional<ItemEntity> try_make_instant_artifact(int making_level);

private:
    static tl::optional<BaseitemKey> try_make_instant_artifact(FixedArtifactId fa_id, const ArtifactDefinition &artifact, int making_level);
    static bool can_make_instant_artifact(FixedArtifactId fa_id, const ArtifactDefinition &artifact);
    static bool evaluate_shallow_fixed_artifact(const ArtifactDefinition &artifact, int making_level);
    static bool evaluate_shallow_baseitem(const ArtifactDefinition &artifactt, int making_level);
};
