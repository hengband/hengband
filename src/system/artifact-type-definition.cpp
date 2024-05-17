#include "system/artifact-type-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"

ArtifactType::ArtifactType()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

/*!
 * @brief アーティファクトが生成可能か否かを確認する
 * @param bi_key 生成しようとするアーティファクトのベースアイテムキー
 * @param level プレイヤーが今いる階層
 */
bool ArtifactType::can_generate(const BaseitemKey &generaing_bi_key) const
{
    if (this->name.empty()) {
        return false;
    }

    if (this->is_generated) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return false;
    }

    return this->bi_key == generaing_bi_key;
}

std::map<FixedArtifactId, ArtifactType> artifacts_info;

ArtifactList ArtifactList::instance{};

ArtifactType ArtifactList::dummy{};

ArtifactList &ArtifactList::get_instance()
{
    return instance;
}

std::map<FixedArtifactId, ArtifactType>::iterator ArtifactList::begin()
{
    return artifacts_info.begin();
}

std::map<FixedArtifactId, ArtifactType>::iterator ArtifactList::end()
{
    return artifacts_info.end();
}

std::map<FixedArtifactId, ArtifactType>::const_iterator ArtifactList::begin() const
{
    return artifacts_info.begin();
}

std::map<FixedArtifactId, ArtifactType>::const_iterator ArtifactList::end() const
{
    return artifacts_info.end();
}

std::map<FixedArtifactId, ArtifactType>::reverse_iterator ArtifactList::rbegin()
{
    return artifacts_info.rbegin();
}

std::map<FixedArtifactId, ArtifactType>::reverse_iterator ArtifactList::rend()
{
    return artifacts_info.rend();
}

std::map<FixedArtifactId, ArtifactType>::const_reverse_iterator ArtifactList::rbegin() const
{
    return artifacts_info.rbegin();
}

std::map<FixedArtifactId, ArtifactType>::const_reverse_iterator ArtifactList::rend() const
{
    return artifacts_info.rend();
}

ArtifactType &ArtifactList::get_artifact(const FixedArtifactId id) const
{
    if (id == FixedArtifactId::NONE) {
        return dummy;
    }

    return artifacts_info.at(id);
}
