﻿#include "load/quest-loader.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "object-enchant/trg-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"

/*!
 * @brief ランダムクエスト情報の読み込み
 * @param なし
 * @return なし
 * @details MAX_TRIES: ランダムクエストのモンスターを確定するために試行する回数 /
 * Maximum number of tries for selection of a proper quest monster
 */
void rd_unique_info(void)
{
    const int MAX_TRIES = 100;
    for (int i = 0; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        r_ptr->max_num = MAX_TRIES;
        if (r_ptr->flags1 & RF1_UNIQUE)
            r_ptr->max_num = 1;
        else if (r_ptr->flags7 & RF7_NAZGUL)
            r_ptr->max_num = MAX_NAZGUL_NUM;
    }
}

errr load_town(void)
{
    u16b max_towns_load;
    rd_u16b(&max_towns_load);
    if (max_towns_load <= max_towns)
        return 0;

    load_note(format(_("町が多すぎる(%u)！", "Too many (%u) towns!"), max_towns_load));
    return 23;
}

errr load_quest_info(u16b *max_quests_load, byte *max_rquests_load)
{
    rd_u16b(max_quests_load);
    if (z_older_than(11, 0, 7))
        *max_rquests_load = 10;
    else
        rd_byte(max_rquests_load);

    if (*max_quests_load <= max_q_idx)
        return 0;

    load_note(format(_("クエストが多すぎる(%u)！", "Too many (%u) quests!"), *max_quests_load));
    return 23;
}

static bool check_quest_index(int loading_quest_index)
{
    if (loading_quest_index < max_q_idx)
        return FALSE;

    strip_bytes(2);
    strip_bytes(2);
    return TRUE;
}

static void load_quest_completion(quest_type *q_ptr)
{
    rd_s16b(&q_ptr->status);
    s16b tmp16s;
    rd_s16b(&tmp16s);
    q_ptr->level = tmp16s;

    if (z_older_than(11, 0, 6))
        q_ptr->complev = 0;
    else {
        byte tmp8u;
        rd_byte(&tmp8u);
        q_ptr->complev = tmp8u;
    }

    if (h_older_than(2, 1, 2, 2))
        q_ptr->comptime = 0;
    else
        rd_u32b(&q_ptr->comptime);
}

static void load_quest_details(player_type *creature_ptr, quest_type *q_ptr, int loading_quest_index)
{
    s16b tmp16s;
    rd_s16b(&tmp16s);
    q_ptr->cur_num = (MONSTER_NUMBER)tmp16s;
    rd_s16b(&tmp16s);
    q_ptr->max_num = (MONSTER_NUMBER)tmp16s;
    rd_s16b(&q_ptr->type);

    rd_s16b(&q_ptr->r_idx);
    if ((q_ptr->type == QUEST_TYPE_RANDOM) && (!q_ptr->r_idx))
        determine_random_questor(creature_ptr, &quest[loading_quest_index]);

    rd_s16b(&q_ptr->k_idx);
    if (q_ptr->k_idx)
        a_info[q_ptr->k_idx].gen_flags |= TRG_QUESTITEM;

    byte tmp8u;
    rd_byte(&tmp8u);
    q_ptr->flags = tmp8u;
}

void analyze_quests(player_type *creature_ptr, const u16b max_quests_load, const byte max_rquests_load)
{
    QUEST_IDX old_inside_quest = creature_ptr->current_floor_ptr->inside_quest;
    for (int i = 0; i < max_quests_load; i++) {
        if (check_quest_index(i))
            continue;

        quest_type *const q_ptr = &quest[i];
        load_quest_completion(q_ptr);
        bool is_quest_running = (q_ptr->status == QUEST_STATUS_TAKEN);
        is_quest_running |= (!z_older_than(10, 3, 14) && (q_ptr->status == QUEST_STATUS_COMPLETED));
        is_quest_running |= (!z_older_than(11, 0, 7) && (i >= MIN_RANDOM_QUEST) && (i <= (MIN_RANDOM_QUEST + max_rquests_load)));
        if (!is_quest_running)
            continue;

        load_quest_details(creature_ptr, q_ptr, i);
        if (z_older_than(10, 3, 11))
            set_zangband_quest(creature_ptr, q_ptr, i, old_inside_quest);
        else {
            byte tmp8u;
            rd_byte(&tmp8u);
            q_ptr->dungeon = tmp8u;
        }

        if (q_ptr->status == QUEST_STATUS_TAKEN || q_ptr->status == QUEST_STATUS_UNTAKEN)
            if (r_info[q_ptr->r_idx].flags1 & RF1_UNIQUE)
                r_info[q_ptr->r_idx].flags1 |= RF1_QUESTOR;
    }
}
