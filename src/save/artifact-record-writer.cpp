#include "save/artifact-record-writer.h"
#include "save/save-util.h"
#include "system/artifact/artifact-record.h"
#include "util/enum-converter.h"
#include <cstdint>

void wr_artifact_records()
{
    const auto &records = ArtifactRecords::get_instance();
    const auto records_size = static_cast<uint16_t>(records.size());
    wr_u16b(records_size);
    for (uint16_t i = 1; i <= records_size; i++) {
        const auto fa_id = i2enum<FixedArtifactId>(i);
        wr_bool(records.get_generated(fa_id));
        wr_bool(records.get_identified(fa_id));
        wr_bool(records.get_known(fa_id));
        wr_bool(records.get_quest_reward(fa_id));
        const auto floor_id = records.get_floor_id(fa_id);
        wr_s16b(floor_id ? *floor_id : 0); // 地上(0F)では固定アーティファクトは生成されないので、0を書き込んでも問題ない.
    }
}
