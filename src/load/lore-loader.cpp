#include "load/lore-loader.h"
#include "game-option/runtime-arguments.h"
#include "monster-race/monster-race.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "load/angband-version-comparer.h"

/*!
 * @brief モンスターの思い出を読み込む / Read the monster lore
 * @param r_ptr 読み込み先モンスター種族情報へのポインタ
 * @param r_idx 読み込み先モンスターID(種族特定用)
 * @return なし
 */
void rd_lore(monster_race *r_ptr, MONRACE_IDX r_idx)
{
    s16b tmp16s;
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
    rd_u32b(&r_ptr->r_flags4);
    rd_u32b(&r_ptr->r_flags5);
    rd_u32b(&r_ptr->r_flags6);
    if (h_older_than(1, 5, 0, 3))
        set_old_lore(r_ptr, r_idx);
    else
        rd_u32b(&r_ptr->r_flagsr);

    rd_byte(&tmp8u);
    r_ptr->max_num = (MONSTER_NUMBER)tmp8u;

    rd_s16b(&r_ptr->floor_id);
    rd_byte(&tmp8u);

    r_ptr->r_flags1 &= r_ptr->flags1;
    r_ptr->r_flags2 &= r_ptr->flags2;
    r_ptr->r_flags3 &= r_ptr->flags3;
    r_ptr->r_flags4 &= r_ptr->flags4;
    r_ptr->r_flags5 &= r_ptr->a_ability_flags1;
    r_ptr->r_flags6 &= r_ptr->a_ability_flags2;
    r_ptr->r_flagsr &= r_ptr->flagsr;
}

errr load_lore(void)
{
    u16b loading_max_r_idx;
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
