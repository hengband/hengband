#include "save/lore-writer.h"
#include "load/old/monster-flag-types-savefile50.h"
#include "save/save-util.h"
#include "system/monster-race-info.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターの思い出を書き込む / Write a "lore" record
 * @param r_idx モンスター種族ID
 */
void wr_lore(MonraceId r_idx)
{
    auto *r_ptr = &monraces_info[r_idx];
    wr_s16b((int16_t)r_ptr->r_sights);
    wr_s16b((int16_t)r_ptr->r_deaths);
    wr_s16b((int16_t)r_ptr->r_pkills);
    wr_s16b((int16_t)r_ptr->r_akills);
    wr_s16b((int16_t)r_ptr->r_tkills);

    wr_byte(r_ptr->r_wake);
    wr_byte(r_ptr->r_ignore);

    byte tmp8u = r_ptr->r_can_evolve ? 1 : 0;
    wr_byte(tmp8u);

    wr_byte((byte)r_ptr->r_drop_gold);
    wr_byte((byte)r_ptr->r_drop_item);

    wr_byte(0); /* unused now */
    wr_byte(r_ptr->r_cast_spell);

    wr_byte(r_ptr->r_blows[0]);
    wr_byte(r_ptr->r_blows[1]);
    wr_byte(r_ptr->r_blows[2]);
    wr_byte(r_ptr->r_blows[3]);

    wr_FlagGroup(r_ptr->r_resistance_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_ability_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_aura_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_behavior_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_kind_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_drop_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_feature_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_special_flags, wr_byte);
    wr_FlagGroup(r_ptr->r_misc_flags, wr_byte);

    wr_byte((byte)r_ptr->max_num);
    wr_s16b(r_ptr->floor_id);

    wr_s16b(r_ptr->defeat_level);
    wr_u32b(r_ptr->defeat_time);
    wr_byte(0);
}
