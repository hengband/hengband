#include "load/lore-loader.h"
#include "game-option/runtime-arguments.h"
#include "monster-race/monster-race.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "load/angband-version-comparer.h"
#include "system/monster-race-definition.h"

/*!
 * @brief モンスターの思い出を読み込む / Read the monster lore
 * @param r_ptr 読み込み先モンスター種族情報へのポインタ
 * @param r_idx 読み込み先モンスターID(種族特定用)
 */
void rd_lore(monster_race *r_ptr, MONRACE_IDX r_idx)
{
    int16_t tmp16s;
    rd_s16b(&tmp16s);
    r_ptr->r_sights = (MONSTER_NUMBER)tmp16s;

    rd_s16b(&tmp16s);
    r_ptr->r_deaths = (MONSTER_NUMBER)tmp16s;

    rd_s16b(&tmp16s);
    r_ptr->r_pkills = (MONSTER_NUMBER)tmp16s;

    if (h_older_than(1, 7, 0, 5)) {
        r_ptr->r_akills = r_ptr->r_pkills;
    } else {
        rd_s16b(&tmp16s);
        r_ptr->r_akills = (MONSTER_NUMBER)tmp16s;
    }

    rd_s16b(&tmp16s);
    r_ptr->r_tkills = (MONSTER_NUMBER)tmp16s;

    rd_byte(&r_ptr->r_wake);
    rd_byte(&r_ptr->r_ignore);
    rd_byte(&r_ptr->r_xtra1);
    rd_byte(&r_ptr->r_xtra2);

    byte tmp8u;
    rd_byte(&tmp8u);
    r_ptr->r_drop_gold = (ITEM_NUMBER)tmp8u;
    rd_byte(&tmp8u);
    r_ptr->r_drop_item = (ITEM_NUMBER)tmp8u;

    rd_byte(&tmp8u);
    rd_byte(&r_ptr->r_cast_spell);

    rd_byte(&r_ptr->r_blows[0]);
    rd_byte(&r_ptr->r_blows[1]);
    rd_byte(&r_ptr->r_blows[2]);
    rd_byte(&r_ptr->r_blows[3]);

    rd_u32b(&r_ptr->r_flags1);
    rd_u32b(&r_ptr->r_flags2);
    rd_u32b(&r_ptr->r_flags3);
    if (loading_savefile_version_is_older_than(3)) {
        uint32_t f4, f5, f6;
        rd_u32b(&f4);
        rd_u32b(&f5);
        rd_u32b(&f6);
        if (h_older_than(1, 5, 0, 3))
            set_old_lore(r_ptr, f4, r_idx);
        else
            rd_u32b(&r_ptr->r_flagsr);

        auto migrate = [r_ptr](uint32_t f, int start_idx) {
            std::bitset<32> flag_bits(f);
            for (size_t i = 0; i < flag_bits.size(); i++) {
                auto ability = static_cast<RF_ABILITY>(start_idx + i);
                r_ptr->r_ability_flags[ability] = flag_bits[i];
            }
        };
        migrate(f4, 0);
        migrate(f5, 32);
        migrate(f6, 64);
    } else {
        rd_u32b(&r_ptr->r_flagsr);
        rd_FlagGroup(r_ptr->r_ability_flags, rd_byte);
    }

    rd_byte(&tmp8u);
    r_ptr->max_num = (MONSTER_NUMBER)tmp8u;

    rd_s16b(&r_ptr->floor_id);

    if (!loading_savefile_version_is_older_than(4)) {
        rd_s16b(&r_ptr->defeat_level);
        rd_u32b(&r_ptr->defeat_time);
    }

    rd_byte(&tmp8u);

    r_ptr->r_flags1 &= r_ptr->flags1;
    r_ptr->r_flags2 &= r_ptr->flags2;
    r_ptr->r_flags3 &= r_ptr->flags3;
    r_ptr->r_flagsr &= r_ptr->flagsr;
    r_ptr->r_ability_flags &= r_ptr->ability_flags;
}

errr load_lore(void)
{
    uint16_t loading_max_r_idx;
    rd_u16b(&loading_max_r_idx);

    monster_race *r_ptr;
    monster_race dummy;
    for (int i = 0; i < loading_max_r_idx; i++) {
        if (i < max_r_idx)
            r_ptr = &r_info[i];
        else
            r_ptr = &dummy;

        rd_lore(r_ptr, (MONRACE_IDX)i);
    }

    if (arg_fiddle)
        load_note(_("モンスターの思い出をロードしました", "Loaded Monster Memory"));

    return 0;
}
