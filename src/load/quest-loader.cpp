#include "load/quest-loader.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "load/savedata-old-flag-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "object-enchant/trg-types.h"
#include "system/angband-exceptions.h"
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

std::tuple<uint16_t, byte> load_quest_info()
{
    auto max_quests_load = rd_u16b();
    byte max_rquests_load;
    if (h_older_than(1, 0, 7)) {
        max_rquests_load = 10;
    } else {
        max_rquests_load = rd_byte();
    }

    return std::make_tuple(max_quests_load, max_rquests_load);
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

static void load_quest_details(PlayerType *player_ptr, quest_type *q_ptr, const QuestId loading_quest_index)
{
    q_ptr->cur_num = rd_s16b();
    q_ptr->max_num = rd_s16b();
    q_ptr->type = i2enum<QuestKindType>(rd_s16b());

    q_ptr->r_idx = i2enum<MonsterRaceId>(rd_s16b());
    if ((q_ptr->type == QuestKindType::RANDOM) && !MonsterRace(q_ptr->r_idx).is_valid()) {
        auto &quest_list = QuestList::get_instance();
        determine_random_questor(player_ptr, &quest_list[loading_quest_index]);
    }
    q_ptr->k_idx = rd_s16b();
    if (q_ptr->k_idx) {
        a_info[q_ptr->k_idx].gen_flags.set(ItemGenerationTraitType::QUESTITEM);
    }

    q_ptr->flags = rd_byte();
}

static bool is_missing_id_ver_16(const QuestId q_idx)
{
    auto is_missing_id = (enum2i(q_idx) == 0);
    is_missing_id |= (enum2i(q_idx) == 13);
    is_missing_id |= (enum2i(q_idx) == 17);
    is_missing_id |= (enum2i(q_idx) >= 35 && enum2i(q_idx) <= 39);
    is_missing_id |= (enum2i(q_idx) >= 88 && enum2i(q_idx) <= 100);

    auto is_deleted_random_quest = (enum2i(q_idx) >= 50 || enum2i(q_idx) <= 88);

    return is_missing_id || is_deleted_random_quest;
}

static bool is_loadable_quest(const QuestId q_idx, const byte max_rquests_load)
{
    const auto &quest_list = QuestList::get_instance();
    if (quest_list.find(q_idx) != quest_list.end()) {
        return true;
    }

    bool is_missing_id;

    if (loading_savefile_version_is_older_than(17)) {
        is_missing_id = is_missing_id_ver_16(q_idx);
    } else {
        is_missing_id = false;
    }

    if (!is_missing_id) {
        const std::string msg(_("削除されたクエストのあるセーブデータはサポート対象外です。",
            "The save data with deleted quests is unsupported."));
        throw SaveDataNotSupportedException(msg);
    }

    auto status = i2enum<QuestStatusType>(rd_s16b());

    strip_bytes(2);
    if (!h_older_than(1, 0, 6)) {
        strip_bytes(1);
    }
    if (!h_older_than(2, 1, 2, 2)) {
        strip_bytes(4);
    }

    auto is_quest_running = (status == QuestStatusType::TAKEN);
    is_quest_running |= (!h_older_than(0, 3, 14) && (status == QuestStatusType::COMPLETED));
    is_quest_running |= (!h_older_than(1, 0, 7) && (enum2i(q_idx) >= MIN_RANDOM_QUEST) && (enum2i(q_idx) <= (MIN_RANDOM_QUEST + max_rquests_load)));
    if (!is_quest_running) {
        return false;
    }

    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(2);
    strip_bytes(1);
    return false;
}

void analyze_quests(PlayerType *player_ptr, const uint16_t max_quests_load, const byte max_rquests_load)
{
    QuestId old_inside_quest = player_ptr->current_floor_ptr->quest_number;
    for (auto i = 0; i < max_quests_load; i++) {
        QuestId q_idx;
        if (loading_savefile_version_is_older_than(17)) {
            q_idx = i2enum<QuestId>(i);
        } else {
            q_idx = i2enum<QuestId>(rd_s16b());
        }
        if (!is_loadable_quest(q_idx, max_rquests_load)) {
            continue;
        }

        auto &quest_list = QuestList::get_instance();
        auto *q_ptr = &quest_list[q_idx];

        if (loading_savefile_version_is_older_than(15)) {
            if (i == enum2i(OldQuestId15::CITY_SEA) && q_ptr->status != QuestStatusType::UNTAKEN) {
                const std::string msg(_("海底都市クエストを受領または解決しているセーブデータはサポート外です。",
                    "The save data with the taken quest of The City beneath the Sea is unsupported."));
                throw(SaveDataNotSupportedException(msg));
            }
        }

        load_quest_completion(q_ptr);
        auto is_quest_running = (q_ptr->status == QuestStatusType::TAKEN);
        is_quest_running |= (!h_older_than(0, 3, 14) && (q_ptr->status == QuestStatusType::COMPLETED));
        is_quest_running |= (!h_older_than(1, 0, 7) && (enum2i(q_idx) >= MIN_RANDOM_QUEST) && (enum2i(q_idx) <= (MIN_RANDOM_QUEST + max_rquests_load)));
        if (!is_quest_running) {
            continue;
        }

        load_quest_details(player_ptr, q_ptr, q_idx);
        if (h_older_than(0, 3, 11)) {
            set_zangband_quest(player_ptr, q_ptr, q_idx, old_inside_quest);
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
