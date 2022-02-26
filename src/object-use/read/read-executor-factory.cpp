#include "object-use/read/read-executor-factory.h"
#include "artifact/fixed-art-types.h"
#include "object-use/read/gbh-shirt-read-executor.h"
#include "object-use/read/parchment-read-executor.h"
#include "object-use/read/ring-power-read-executor.h"
#include "object-use/read/scroll-read-executor.h"
#include "object/tval-types.h"
#include "system/object-type-definition.h"

std::shared_ptr<ReadExecutorBase> ReadExecutorFactory::create(PlayerType *player_ptr, ObjectType *o_ptr, bool known)
{
    if (o_ptr->tval == ItemKindType::SCROLL) {
        return std::make_shared<ScrollReadExecutor>(player_ptr, o_ptr, known);
    }

    if (o_ptr->fixed_artifact_idx == ART_GHB) {
        return std::make_shared<GbhShirtReadExecutor>();
    }

    if (o_ptr->fixed_artifact_idx == ART_POWER) {
        return std::make_shared<RingOfPowerReadExecutor>();
    }

    if (o_ptr->tval == ItemKindType::PARCHMENT) {
        return std::make_shared<ParchmentReadExecutor>(player_ptr, o_ptr);
    }

    throw("Invalid item is specified; this can't be read!");
}
