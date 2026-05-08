#include "system/artifact/artifact-list.h"
#include "artifact/fixed-art-types.h"
#include "system/artifact/artifact-definition.h"
#include "util/enum-converter.h"

ArtifactList ArtifactList::instance{};

ArtifactDefinition ArtifactList::dummy{};

ArtifactList &ArtifactList::get_instance()
{
    return instance;
}

const ArtifactDefinition &ArtifactList::get_artifact(const FixedArtifactId fa_id) const
{
    if (fa_id == FixedArtifactId::NONE) {
        return dummy;
    }

    return this->artifacts.at(fa_id);
}

bool ArtifactList::order(const FixedArtifactId id1, const FixedArtifactId id2) const
{
    const auto &artifact1 = this->get_artifact(id1);
    const auto &artifact2 = this->get_artifact(id2);
    if (artifact1.bi_key < artifact2.bi_key) {
        return true;
    }

    if (artifact1.bi_key > artifact2.bi_key) {
        return false;
    }

    if (artifact1.level < artifact2.level) {
        return true;
    }

    if (artifact1.level > artifact2.level) {
        return false;
    }

    return id1 < id2;
}

void ArtifactList::emplace(const FixedArtifactId fa_id, ArtifactDefinition &&artifact)
{
    this->artifacts.emplace(fa_id, std::move(artifact));
}

std::string ArtifactList::get_full_name(const FixedArtifactId fa_id) const
{
    this->validate_fa_id(fa_id);
    if (fa_id == FixedArtifactId::NONE) {
        return "";
    }

    return this->artifacts.at(fa_id).build_full_name();
}

void ArtifactList::validate_fa_id(const FixedArtifactId fa_id) const
{
    if (fa_id < FixedArtifactId::NONE || fa_id > i2enum<FixedArtifactId>(this->artifacts.size())) {
        THROW_EXCEPTION(std::out_of_range, "Invalid FixedArtifactId: " + std::to_string(static_cast<int>(fa_id)));
    }
}
