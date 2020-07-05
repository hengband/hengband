#include "savedata/world-loader.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/load-util.h"
#include "savedata/load-zangband.h"
#include "world/world.h"

static void rd_hengband_dungeons(void)
{
    byte max = (byte)current_world_ptr->max_d_idx;
    rd_byte(&max);
    s16b tmp16s;
    for (int i = 0; i < max; i++) {
        rd_s16b(&tmp16s);
        max_dlv[i] = tmp16s;
        if (max_dlv[i] > d_info[i].maxdepth)
            max_dlv[i] = d_info[i].maxdepth;
    }
}

void rd_dungeons(player_type *creature_ptr)
{
    if (z_older_than(10, 3, 8))
        rd_zangband_dungeon();
    else
        rd_hengband_dungeons();

    if (creature_ptr->max_plv < creature_ptr->lev)
        creature_ptr->max_plv = creature_ptr->lev;
}

/*!
 * @brief 現実変容処理の有無及びその残りターン数を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void rd_alter_reality(player_type *creature_ptr)
{
    s16b tmp16s;
    if (z_older_than(10, 3, 8))
        creature_ptr->recall_dungeon = DUNGEON_ANGBAND;
    else {
        rd_s16b(&tmp16s);
        creature_ptr->recall_dungeon = (byte)tmp16s;
    }

    if (h_older_than(1, 5, 0, 0))
        creature_ptr->alter_reality = 0;
    else
        rd_s16b(&creature_ptr->alter_reality);
}

void set_gambling_monsters(void)
{
    const int max_gambling_monsters = 4;
    for (int i = 0; i < max_gambling_monsters; i++) {
        rd_s16b(&battle_mon[i]);
        if (z_older_than(10, 3, 4))
            set_zangband_gambling_monsters(i);
        else
            rd_u32b(&mon_odds[i]);
    }
}

/*!
 * @details 自動拾い関係はこれしかないのでworldに突っ込むことにする。必要があれば再分割する
 */
void rd_autopick(player_type *creature_ptr)
{
    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->autopick_autoregister = tmp8u != 0;
}
