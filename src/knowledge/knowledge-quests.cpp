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
#include "system/artifact/artifact-definition.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-list.h"
#include "system/floor/floor-info.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <fmt/format.h>
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
    int total = 0;

    fmt::println(fff, _("《遂行中のクエスト》", "< Current Quest >"));
    const auto disclosed_random_quest_id = quests.find_shallowest_random_quest_id();
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

        populate_quest_text_lines(quest_id);
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
                        note = fmt::format(" - {} 体の{}を倒す。(あと {} 体)", quest.max_num, monrace.name, quest.max_num - quest.cur_num);
#else
                        const auto monster_name = pluralize(monrace.name);
                        note = fmt::format(" - kill {} {}, have killed {}.", quest.max_num, monster_name, quest.cur_num);
#endif
                    } else {
                        note = fmt::format(_(" - {}を倒す。", " - kill {}."), monrace.name);
                    }

                    break;
                }
                case QuestKindType::FIND_ARTIFACT: {
                    std::string item_name("");
                    if (quest.has_reward()) {
                        ItemEntity item(quest.get_reward_bi_id());
                        item.fa_id = quest.get_reward().value_or(FixedArtifactId::NONE);
                        item.set_identification_flag(IdentificationFlag::STORE);
                        item_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
                    }

                    note = fmt::format(_("\n   - {}を見つけ出す。", "\n   - Find {}."), item_name);
                    break;
                }
                case QuestKindType::FIND_EXIT:
                    note = _(" - 出口に到達する。", " - Reach exit.");
                    break;
                case QuestKindType::KILL_NUMBER:
#ifdef JP
                    note = fmt::format(" - {} 体のモンスターを倒す。(あと {} 体)", quest.max_num, quest.max_num - quest.cur_num);
#else
                    note = fmt::format(" - Kill {} monsters, have killed {}.", quest.max_num, quest.cur_num);
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

            fmt::println(fff, _("  {} (危険度:{}階相当){}", "  {} (Danger level: {}){}"), quest.name, (int)quest.level, note);
            if (quest.status == QuestStatusType::COMPLETED) {
                fputs(_("    クエスト達成 - まだ報酬を受けとってない。\n", "    Quest Completed - Unrewarded\n"), fff);
                continue;
            }

            for (const auto &line : quest_text_lines) {
                fmt::println(fff, "    {}", line);
            }

            continue;
        }

        if (quest_id != disclosed_random_quest_id) {
            continue;
        }

        const auto &monrace = quest.get_bounty();
        constexpr auto mes = _("  {} ({} 階) - {}を倒す。\n", "  {} (Dungeon level: {})\n  Kill {}.\n");
        rand_tmp_str = fmt::format(mes, quest.name, quest.level, monrace.name);
    }

    if (!rand_tmp_str.empty()) {
        fputs(rand_tmp_str.data(), fff);
    }

    if (!total) {
        fmt::println(fff, _("  なし", "  Nothing."));
    }
}

static bool do_cmd_knowledge_quests_aux(PlayerType *, FILE *fff, QuestId q_idx)
{
    const auto &quests = QuestList::get_instance();
    const auto &quest = quests.get_quest(q_idx);

    auto is_fixed_quest = QuestType::is_fixed(q_idx);
    if (is_fixed_quest) {
        if (quest.flags & QUEST_FLAG_SILENT) {
            return false;
        }
    }

    const auto playtime_str = fmt::format("{:02}:{:02}:{:02}", quest.comptime / (60 * 60), (quest.comptime / 60) % 60, quest.comptime % 60);
    auto fputs_name_remain = [fff](const auto &name) {
        for (auto i = 1U; i < name.size(); ++i) {
            fmt::println(fff, "  {}", name[i]);
        }
    };

    if (is_fixed_quest || !quest.get_bounty().is_valid()) {
        auto name = str_separate(quest.name, 35);
        constexpr auto mes = _("  {:<35} (危険度:{:3}階相当) - レベル{:2} - {}\n", "  {:<35} (Danger  level: {:3}) - level {:2} - {}\n");
        fmt::print(fff, mes, fmt::bytes(name.front()), (int)quest.level, quest.complev, playtime_str);
        fputs_name_remain(name);
        return true;
    }

    const auto name = str_separate(quest.get_bounty().name, 35);
    if (quest.complev == 0) {
        constexpr auto mes = _("  {:<35} ({:3}階)            -   不戦勝 - {}\n", "  {:<35} (Dungeon level: {:3}) - Unearned - {}\n");
        fmt::print(fff, mes, fmt::bytes(name.front()), (int)quest.level, playtime_str);
        fputs_name_remain(name);
        return true;
    }

    constexpr auto mes = _("  {:<35} ({:3}階)            - レベル{:2} - {}\n", "  {:<35} (Dungeon level: {:3}) - level {:2} - {}\n");
    fmt::print(fff, mes, fmt::bytes(name.front()), (int)quest.level, quest.complev, playtime_str);
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
    fmt::println(fff, _("《達成したクエスト》", "< Completed Quest >"));
    int16_t total = 0;
    for (const auto quest_id : quest_ids) {
        const auto &quests = QuestList::get_instance();
        const auto &quest = quests.get_quest(quest_id);
        if (quest.status == QuestStatusType::FINISHED && do_cmd_knowledge_quests_aux(player_ptr, fff, quest_id)) {
            ++total;
        }
    }

    if (total == 0) {
        fmt::println(fff, _("  なし", "  Nothing."));
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
    fmt::println(fff, _("《失敗したクエスト》", "< Failed Quest >"));
    int16_t total = 0;
    for (const auto quest_id : quest_ids) {
        const auto &quests = QuestList::get_instance();
        const auto &quest = quests.get_quest(quest_id);
        if (((quest.status == QuestStatusType::FAILED_DONE) || (quest.status == QuestStatusType::FAILED)) && do_cmd_knowledge_quests_aux(player_ptr, fff, quest_id)) {
            ++total;
        }
    }

    if (total == 0) {
        fmt::println(fff, _("  なし", "  Nothing."));
    }
}

/*
 * Print all random quests
 */
static void do_cmd_knowledge_quests_wiz_random(FILE *fff)
{
    fmt::println(fff, _("《残りのランダムクエスト》", "< Remaining Random Quest >"));
    const auto &quests = QuestList::get_instance();
    int16_t total = 0;
    for (const auto &[q_idx, quest] : quests) {
        if (quest.flags & QUEST_FLAG_SILENT) {
            continue;
        }

        if ((quest.type == QuestKindType::RANDOM) && (quest.status == QuestStatusType::TAKEN)) {
            total++;
            constexpr auto mes = _("  {} ({}階, {})\n", "  {} ({}, {})\n");
            fmt::print(fff, mes, quest.name, (int)quest.level, quest.get_bounty().name);
        }
    }

    if (total == 0) {
        fmt::println(fff, _("  なし", "  Nothing."));
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
