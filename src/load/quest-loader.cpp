#include "load/quest-loader.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/quest.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "load/savedata-old-flag-types.h"
#include "object-enchant/trg-types.h"
#include "system/angband-exceptions.h"
#include "system/artifact-type-definition.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"

errr load_town(void)
{
    auto max_towns_load = rd_u16b();
    if (max_towns_load <= towns_info.size()) {
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

static void load_quest_completion(QuestType *q_ptr)
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

static void load_quest_details(PlayerType *player_ptr, QuestType *q_ptr, const QuestId loading_quest_id)
{
    q_ptr->cur_num = rd_s16b();
    q_ptr->max_num = rd_s16b();
    q_ptr->type = i2enum<QuestKindType>(rd_s16b());

    q_ptr->r_idx = i2enum<MonraceId>(rd_s16b());
    if ((q_ptr->type == QuestKindType::RANDOM) && !q_ptr->get_bounty().is_valid()) {
        auto &quests = QuestList::get_instance();
        determine_random_questor(player_ptr, quests.get_quest(loading_quest_id));
    }
    q_ptr->reward_fa_id = i2enum<FixedArtifactId>(rd_s16b());
    if (q_ptr->has_reward()) {
        q_ptr->get_reward().gen_flags.set(ItemGenerationTraitType::QUESTITEM);
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
    const auto &quests = QuestList::get_instance();
    if (quests.find(q_idx) != quests.end()) {
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
        QuestId quest_id;
        if (loading_savefile_version_is_older_than(17)) {
            quest_id = i2enum<QuestId>(i);
        } else {
            quest_id = i2enum<QuestId>(rd_s16b());
        }
        if (!is_loadable_quest(quest_id, max_rquests_load)) {
            continue;
        }

        auto &quests = QuestList::get_instance();
        auto &quest = quests.get_quest(quest_id);

        if (loading_savefile_version_is_older_than(15)) {
            if (i == enum2i(OldQuestId15::CITY_SEA) && quest.status != QuestStatusType::UNTAKEN) {
                const std::string msg(_("海底都市クエストを受領または解決しているセーブデータはサポート外です。",
                    "The save data with the taken quest of The City beneath the Sea is unsupported."));
                throw(SaveDataNotSupportedException(msg));
            }
        }

        load_quest_completion(&quest);
        auto is_quest_running = (quest.status == QuestStatusType::TAKEN);
        is_quest_running |= (!h_older_than(0, 3, 14) && (quest.status == QuestStatusType::COMPLETED));
        is_quest_running |= (!h_older_than(1, 0, 7) && (enum2i(quest_id) >= MIN_RANDOM_QUEST) && (enum2i(quest_id) <= (MIN_RANDOM_QUEST + max_rquests_load)));
        if (!is_quest_running) {
            continue;
        }

        load_quest_details(player_ptr, &quest, quest_id);
        if (h_older_than(0, 3, 11)) {
            set_zangband_quest(player_ptr, &quest, quest_id, old_inside_quest);
        } else {
            quest.dungeon = rd_byte();
        }

        if (quest.status == QuestStatusType::TAKEN || quest.status == QuestStatusType::UNTAKEN) {
            auto &monrace = quest.get_bounty();
            if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
                monrace.misc_flags.set(MonsterMiscType::QUESTOR);
            }
        }
    }
}
