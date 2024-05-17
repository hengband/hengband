#include "system/artifact-type-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"

ArtifactType::ArtifactType()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

std::map<FixedArtifactId, ArtifactType> artifacts_info;

ArtifactList ArtifactList::instance{};

ArtifactType ArtifactList::dummy{};

ArtifactList &ArtifactList::get_instance()
{
    return instance;
}

ArtifactType &ArtifactList::get_artifact(const FixedArtifactId id) const
{
    if (id == FixedArtifactId::NONE) {
        return dummy;
    }

    return artifacts_info.at(id);
}
