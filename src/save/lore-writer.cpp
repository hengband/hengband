#include "save/lore-writer.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "save/save-util.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターの思い出を書き込む / Write a "lore" record
 * @param r_idx モンスター種族ID
 */
void wr_lore(MonraceId monrace_id)
{
    const auto &monrace = MonraceList::get_instance().get_monrace(monrace_id);
    wr_s16b(static_cast<short>(monrace.r_sights));
    wr_s16b(static_cast<short>(monrace.r_deaths));
    wr_s16b(static_cast<short>(monrace.r_pkills));
    wr_s16b(static_cast<short>(monrace.r_akills));
    wr_s16b(static_cast<short>(monrace.r_tkills));

    wr_byte(monrace.r_wake);
    wr_byte(monrace.r_ignore);

    uint8_t tmp8u = monrace.r_can_evolve ? 1 : 0;
    wr_byte(tmp8u);

    wr_byte(static_cast<uint8_t>(monrace.r_drop_gold));
    wr_byte(static_cast<uint8_t>(monrace.r_drop_item));

    wr_byte(0); /* unused now */
    wr_byte(monrace.r_cast_spell);

    wr_byte(monrace.r_blows[0]);
    wr_byte(monrace.r_blows[1]);
    wr_byte(monrace.r_blows[2]);
    wr_byte(monrace.r_blows[3]);

    wr_FlagGroup(monrace.r_resistance_flags, wr_byte);
    wr_FlagGroup(monrace.r_ability_flags, wr_byte);
    wr_FlagGroup(monrace.r_aura_flags, wr_byte);
    wr_FlagGroup(monrace.r_behavior_flags, wr_byte);
    wr_FlagGroup(monrace.r_kind_flags, wr_byte);
    wr_FlagGroup(monrace.r_drop_flags, wr_byte);
    wr_FlagGroup(monrace.r_feature_flags, wr_byte);
    wr_FlagGroup(monrace.r_special_flags, wr_byte);
    wr_FlagGroup(monrace.r_misc_flags, wr_byte);

    wr_byte(static_cast<uint8_t>(monrace.max_num));
    wr_s16b(monrace.floor_id);

    wr_s16b(monrace.defeat_level);
    wr_u32b(monrace.defeat_time);
    wr_byte(0);
}
