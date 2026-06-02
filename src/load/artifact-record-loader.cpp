#include "load/artifact-record-loader.h"
#include "load/load-util.h"
#include "system/artifact/artifact-record.h"
#include "util/enum-converter.h"
#include <cstdint>
#include <tl/optional.hpp>

void rd_artifact_records()
{
    const auto tmp16s = rd_s16b();
    auto &records = ArtifactRecords::get_instance();
    for (int16_t i = 1; i <= tmp16s; i++) {
        const auto fa_id = i2enum<FixedArtifactId>(i);
        records.set_generated(fa_id, rd_bool());
        records.set_identified(fa_id, rd_bool());
        records.set_known(fa_id, rd_bool());
        records.set_quest_reward(fa_id, rd_bool());
        const auto floor_id = rd_s16b();
        records.set_floor_id(fa_id, (floor_id > 0) ? tl::optional<int16_t>(floor_id) : tl::nullopt);
    }

    load_note(_("伝説のアイテムをロードしました", "Loaded Artifacts"));
}
