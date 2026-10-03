#pragma once

#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include <map>

namespace test {

/*!
 * @brief 固定アーティファクト定義を一時退避し、スコープ終了時に元の定義表へ戻す
 */
class ArtifactListTestAccess {
public:
    ArtifactListTestAccess()
        : artifacts(ArtifactList::get_instance())
    {
        this->artifacts.artifacts.swap(this->previous_artifacts);
    }

    ArtifactListTestAccess(const ArtifactListTestAccess &) = delete;
    ArtifactListTestAccess &operator=(const ArtifactListTestAccess &) = delete;

    ~ArtifactListTestAccess()
    {
        this->artifacts.artifacts.swap(this->previous_artifacts);
    }

private:
    ArtifactList &artifacts;
    std::map<FixedArtifactId, ArtifactDefinition> previous_artifacts;
};

}
