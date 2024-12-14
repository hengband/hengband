/*!
 * @brief 既知のクエストを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-quests.h"
#include "artifact/fixed-art-types.h"
#include "core/show-file.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "info-reader/fixed-map-parser.h"
#include "io-dump/dump-util.h"
#include "locale/english.h"
#include "object-enchant/special-object-flags.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <numeric>

/*!
 * @brief Check on the status of an active quest
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_checkquest(PlayerType *player_ptr)
{
    screen_save();
    do_cmd_knowledge_quests(player_ptr);
    screen_load();
}

/*!
 * @brief Print all active quests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @todo PlayerTypeではなくQUEST_IDXを引数にすべきかもしれない
 */
static void do_cmd_knowledge_quests_current(PlayerType *player_ptr, FILE *fff)
{
    const auto &quests = QuestList::get_instance();
    std::string rand_tmp_str;
    int rand_level = 100;
    int total = 0;

    fprintf(fff, _("《遂行中のクエスト》\n", "< Current Quest >\n"));
    const auto &dungeon_records = DungeonRecords::get_instance();
    for (const auto &[quest_id, quest] : quests) {
        if (quest_id == QuestId::NONE) {
            continue;
        }

        auto is_print = quest.status == QuestStatusType::TAKEN;
        is_print |= (quest.status == QuestStatusType::STAGE_COMPLETED) && (quest.type == QuestKindType::TOWER);
        is_print |= quest.status == QuestStatusType::COMPLETED;
        if (!is_print) {
            continue;
        }

        const auto old_quest = player_ptr->current_floor_ptr->quest_number;

        quest_text_lines.clear();

        player_ptr->current_floor_ptr->quest_number = quest_id;
        init_flags = INIT_SHOW_TEXT;
        parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        player_ptr->current_floor_ptr->quest_number = old_quest;
        if (quest.flags & QUEST_FLAG_SILENT) {
            continue;
        }

        total++;
        if (quest.type != QuestKindType::RANDOM) {
            std::string note;
            if (quest.status == QuestStatusType::TAKEN || quest.status == QuestStatusType::STAGE_COMPLETED) {
                switch (quest.type) {
                case QuestKindType::KILL_LEVEL: {
                    const auto &monrace = quest.get_bounty();
                    if (quest.max_num > 1) {
#ifdef JP
                        note = format(" - %d 体の%sを倒す。(あと %d 体)", (int)quest.max_num, monrace.name.data(), (int)(quest.max_num - quest.cur_num));
#else
                        const auto monster_name = pluralize(monrace.name);
                        note = format(" - kill %d %s, have killed %d.", (int)quest.max_num, monster_name.data(), (int)quest.cur_num);
#endif
                    } else {
                        note = format(_(" - %sを倒す。", " - kill %s."), monrace.name.data());
                    }

                    break;
                }
                case QuestKindType::FIND_ARTIFACT: {
                    std::string item_name("");
                    if (quest.has_reward()) {
                        const auto &artifact = quest.get_reward();
                        ItemEntity item(artifact.bi_key);
                        item.fa_id = quest.reward_fa_id;
                        item.ident = IDENT_STORE;
                        item_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
                    }

                    note = format(_("\n   - %sを見つけ出す。", "\n   - Find %s."), item_name.data());
                    break;
                }
                case QuestKindType::FIND_EXIT:
                    note = _(" - 出口に到達する。", " - Reach exit.");
                    break;
                case QuestKindType::KILL_NUMBER:
#ifdef JP
                    note = format(" - %d 体のモンスターを倒す。(あと %d 体)", (int)quest.max_num, (int)(quest.max_num - quest.cur_num));
#else
                    note = format(" - Kill %d monsters, have killed %d.", (int)quest.max_num, (int)quest.cur_num);
#endif
                    break;

                case QuestKindType::KILL_ALL:
                case QuestKindType::TOWER:
                    note = _(" - 全てのモンスターを倒す。", " - Kill all monsters.");
                    break;
                default:
                    break;
                }
            }

            fprintf(fff, _("  %s (危険度:%d階相当)%s\n", "  %s (Danger level: %d)%s\n"), quest.name.data(), (int)quest.level, note.data());
            if (quest.status == QuestStatusType::COMPLETED) {
                fputs(_("    クエスト達成 - まだ報酬を受けとってない。\n", "    Quest Completed - Unrewarded\n"), fff);
                continue;
            }

            for (const auto &line : quest_text_lines) {
                fprintf(fff, "    %s\n", line.data());
            }

            continue;
        }

        if (quest.level >= rand_level) {
            continue;
        }
        rand_level = quest.level;
        if (dungeon_records.get_record(DungeonId::ANGBAND).get_max_level() < rand_level) {
            continue;
        }

        const auto &monrace = quest.get_bounty();
        if (quest.max_num <= 1) {
            constexpr auto mes = _("  %s (%d 階) - %sを倒す。\n", "  %s (Dungeon level: %d)\n  Kill %s.\n");
            rand_tmp_str = format(mes, quest.name.data(), (int)quest.level, monrace.name.data());
            continue;
        }
    }

    if (!rand_tmp_str.empty()) {
        fputs(rand_tmp_str.data(), fff);
    }

    if (!total) {
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
    }
}

static bool do_cmd_knowledge_quests_aux(PlayerType *player_ptr, FILE *fff, QuestId q_idx)
{
    const auto &quests = QuestList::get_instance();
    const auto &quest = quests.get_quest(q_idx);

    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto is_fixed_quest = QuestType::is_fixed(q_idx);
    if (is_fixed_quest) {
        QuestId old_quest = floor_ptr->quest_number;
        floor_ptr->quest_number = q_idx;
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        floor_ptr->quest_number = old_quest;
        if (quest.flags & QUEST_FLAG_SILENT) {
            return false;
        }
    }

    const auto playtime_str = format("%02d:%02d:%02d", quest.comptime / (60 * 60), (quest.comptime / 60) % 60, quest.comptime % 60);
    auto fputs_name_remain = [fff](const auto &name) {
        for (auto i = 1U; i < name.size(); ++i) {
            fprintf(fff, "  %s\n", name[i].data());
        }
    };

    if (is_fixed_quest || !quest.get_bounty().is_valid()) {
        auto name = str_separate(quest.name, 35);
        constexpr auto mes = _("  %-35s (危険度:%3d階相当) - レベル%2d - %s\n", "  %-35s (Danger  level: %3d) - level %2d - %s\n");
        fprintf(fff, mes, name.front().data(), (int)quest.level, quest.complev, playtime_str.data());
        fputs_name_remain(name);
        return true;
    }

    const auto name = str_separate(quest.get_bounty().name, 35);
    if (quest.complev == 0) {
        constexpr auto mes = _("  %-35s (%3d階)            -   不戦勝 - %s\n", "  %-35s (Dungeon level: %3d) - Unearned - %s\n");
        fprintf(fff, mes, name.front().data(), (int)quest.level, playtime_str.data());
        fputs_name_remain(name);
        return true;
    }

    const auto mes = _("  %-35s (%3d階)            - レベル%2d - %s\n", "  %-35s (Dungeon level: %3d) - level %2d - %s\n");
    fprintf(fff, mes, name.front().data(), (int)quest.level, quest.complev, playtime_str.data());
    fputs_name_remain(name);
    return true;
}

/*
 * Print all finished quests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff セーブファイル (展開済？)
 * @param quest_ids 受注したことのあるクエスト群
 */
void do_cmd_knowledge_quests_completed(PlayerType *player_ptr, FILE *fff, const std::vector<QuestId> &quest_ids)
{
    fprintf(fff, _("《達成したクエスト》\n", "< Completed Quest >\n"));
    int16_t total = 0;
    for (const auto quest_id : quest_ids) {
        const auto &quests = QuestList::get_instance();
        const auto &quest = quests.get_quest(quest_id);
        if (quest.status == QuestStatusType::FINISHED && do_cmd_knowledge_quests_aux(player_ptr, fff, quest_id)) {
            ++total;
        }
    }

    if (total == 0) {
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
    }
}

/*
 * Print all failed quests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff セーブファイル (展開済？)
 * @param quest_ids 受注したことのあるクエスト群
 */
void do_cmd_knowledge_quests_failed(PlayerType *player_ptr, FILE *fff, const std::vector<QuestId> &quest_ids)
{
    fprintf(fff, _("《失敗したクエスト》\n", "< Failed Quest >\n"));
    int16_t total = 0;
    for (const auto quest_id : quest_ids) {
        const auto &quests = QuestList::get_instance();
        const auto &quest = quests.get_quest(quest_id);
        if (((quest.status == QuestStatusType::FAILED_DONE) || (quest.status == QuestStatusType::FAILED)) && do_cmd_knowledge_quests_aux(player_ptr, fff, quest_id)) {
            ++total;
        }
    }

    if (total == 0) {
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
    }
}

/*
 * Print all random quests
 */
static void do_cmd_knowledge_quests_wiz_random(FILE *fff)
{
    fprintf(fff, _("《残りのランダムクエスト》\n", "< Remaining Random Quest >\n"));
    const auto &quests = QuestList::get_instance();
    int16_t total = 0;
    for (const auto &[q_idx, quest] : quests) {
        if (quest.flags & QUEST_FLAG_SILENT) {
            continue;
        }

        if ((quest.type == QuestKindType::RANDOM) && (quest.status == QuestStatusType::TAKEN)) {
            total++;
            constexpr auto mes = _("  %s (%d階, %s)\n", "  %s (%d, %s)\n");
            fprintf(fff, mes, quest.name.data(), (int)quest.level, quest.get_bounty().name.data());
        }
    }

    if (total == 0) {
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
    }
}

/*
 * Print quest status of all active quests
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_knowledge_quests(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    const auto &quests = QuestList::get_instance();
    const auto quest_ids = quests.get_sorted_quest_ids();
    do_cmd_knowledge_quests_current(player_ptr, fff);
    fputc('\n', fff);
    do_cmd_knowledge_quests_completed(player_ptr, fff, quest_ids);
    fputc('\n', fff);
    do_cmd_knowledge_quests_failed(player_ptr, fff, quest_ids);
    if (AngbandWorld::get_instance().wizard) {
        fputc('\n', fff);
        do_cmd_knowledge_quests_wiz_random(fff);
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("クエスト達成状況", "Quest status"));
    fd_kill(file_name);
}
