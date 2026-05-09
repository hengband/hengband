#include "system/artifact/artifact-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"

ArtifactDefinition::ArtifactDefinition()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

bool ArtifactDefinition::can_generate(const BaseitemKey &generating_bi_key) const
{
    if (this->gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
        return false;
    }

    if (this->is_instant_artifact()) {
        return false;
    }

    return this->bi_key == generating_bi_key;
}

bool ArtifactDefinition::is_instant_artifact() const
{
    return this->gen_flags.has(ItemGenerationTraitType::INSTA_ART);
}
