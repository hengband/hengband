#include "load/quest-loader.h"
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
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

errr load_town(void)
{
    auto max_towns_load = rd_u16b();
    if (max_towns_load <= max_towns) {
        return 0;
    }

    load_note(format(_("町が多すぎる(%u)！", "Too many (%u) towns!"), max_towns_load));
    return 23;
}

errr load_quest_info(uint16_t *max_quests_load, byte *max_rquests_load)
{
    *max_quests_load = rd_u16b();
    if (h_older_than(1, 0, 7)) {
        *max_rquests_load = 10;
    } else {
        *max_rquests_load = rd_byte();
    }

    if (*max_quests_load <= max_q_idx) {
        return 0;
    }

    load_note(format(_("クエストが多すぎる(%u)！", "Too many (%u) quests!"), *max_quests_load));
    return 23;
}

static bool check_quest_index(int loading_quest_index)
{
    if (loading_quest_index < max_q_idx) {
        return false;
    }

    strip_bytes(2);
    strip_bytes(2);
    return true;
}

static void load_quest_completion(quest_type *q_ptr)
{
    q_ptr->status = i2enum<QuestStatusType>(rd_s16b());
    q_ptr->level = rd_s16b();

    if (h_older_than(1, 0, 6)) {
        q_ptr->complev = 0;
    } else {
        q_ptr->complev = rd_byte();
    }

    if (h_older_than(2, 1, 2, 2)) {
        q_ptr->comptime = 0;
    } else {
        q_ptr->comptime = rd_u32b();
    }
}

static void load_quest_details(PlayerType *player_ptr, quest_type *q_ptr, int loading_quest_index)
{
    q_ptr->cur_num = rd_s16b();
    q_ptr->max_num = rd_s16b();
    q_ptr->type = i2enum<QuestKindType>(rd_s16b());

    q_ptr->r_idx = rd_s16b();
    if ((q_ptr->type == QuestKindType::RANDOM) && (!q_ptr->r_idx)) {
        determine_random_questor(player_ptr, &quest[i2enum<QuestId>(loading_quest_index)]);
    }
    q_ptr->k_idx = rd_s16b();
    if (q_ptr->k_idx) {
        a_info[q_ptr->k_idx].gen_flags.set(ItemGenerationTraitType::QUESTITEM);
    }

    q_ptr->flags = rd_byte();
}

void analyze_quests(PlayerType *player_ptr, const uint16_t max_quests_load, const byte max_rquests_load)
{
    QuestId old_inside_quest = player_ptr->current_floor_ptr->quest_number;
    for (int i = 0; i < max_quests_load; i++) {
        if (check_quest_index(i)) {
            continue;
        }

        auto *const q_ptr = &quest[i2enum<QuestId>(i)];
        load_quest_completion(q_ptr);
        bool is_quest_running = (q_ptr->status == QuestStatusType::TAKEN);
        is_quest_running |= (!h_older_than(0, 3, 14) && (q_ptr->status == QuestStatusType::COMPLETED));
        is_quest_running |= (!h_older_than(1, 0, 7) && (i >= MIN_RANDOM_QUEST) && (i <= (MIN_RANDOM_QUEST + max_rquests_load)));
        if (!is_quest_running) {
            continue;
        }

        load_quest_details(player_ptr, q_ptr, i);
        if (h_older_than(0, 3, 11)) {
            set_zangband_quest(player_ptr, q_ptr, i, old_inside_quest);
        } else {
            q_ptr->dungeon = rd_byte();
        }

        if (q_ptr->status == QuestStatusType::TAKEN || q_ptr->status == QuestStatusType::UNTAKEN) {
            if (r_info[q_ptr->r_idx].kind_flags.has(MonsterKindType::UNIQUE)) {
                r_info[q_ptr->r_idx].flags1 |= RF1_QUESTOR;
            }
        }
    }
}
