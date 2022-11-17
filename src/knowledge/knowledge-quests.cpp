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
#include "monster-race/monster-race.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-kind-hook.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include "util/sort.h"
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
    const auto &quest_list = QuestList::get_instance();
    char tmp_str[1024];
    char rand_tmp_str[512] = "\0";
    GAME_TEXT name[MAX_NLEN];
    MonsterRaceInfo *r_ptr;
    int rand_level = 100;
    int total = 0;

    fprintf(fff, _("《遂行中のクエスト》\n", "< Current Quest >\n"));

    for (const auto &[q_idx, q_ref] : quest_list) {
        bool is_print = q_ref.status == QuestStatusType::TAKEN;
        is_print |= (q_ref.status == QuestStatusType::STAGE_COMPLETED) && (q_ref.type == QuestKindType::TOWER);
        is_print |= q_ref.status == QuestStatusType::COMPLETED;
        if (!is_print) {
            continue;
        }

        QuestId old_quest = player_ptr->current_floor_ptr->quest_number;
        for (int j = 0; j < 10; j++) {
            quest_text[j][0] = '\0';
        }

        quest_text_line = 0;
        player_ptr->current_floor_ptr->quest_number = q_idx;
        init_flags = INIT_SHOW_TEXT;
        parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        player_ptr->current_floor_ptr->quest_number = old_quest;
        if (q_ref.flags & QUEST_FLAG_SILENT) {
            continue;
        }
        total++;
        if (q_ref.type != QuestKindType::RANDOM) {
            char note[512] = "\0";
            if (q_ref.status == QuestStatusType::TAKEN || q_ref.status == QuestStatusType::STAGE_COMPLETED) {
                switch (q_ref.type) {
                case QuestKindType::KILL_LEVEL:
                    r_ptr = &monraces_info[q_ref.r_idx];
                    strcpy(name, r_ptr->name.data());
                    if (q_ref.max_num > 1) {
#ifdef JP
                        sprintf(note, " - %d 体の%sを倒す。(あと %d 体)", (int)q_ref.max_num, name, (int)(q_ref.max_num - q_ref.cur_num));
#else
                        plural_aux(name);
                        sprintf(note, " - kill %d %s, have killed %d.", (int)q_ref.max_num, name, (int)q_ref.cur_num);
#endif
                    } else {
                        sprintf(note, _(" - %sを倒す。", " - kill %s."), name);
                    }

                    break;
                case QuestKindType::FIND_ARTIFACT:
                    if (q_ref.reward_artifact_idx != FixedArtifactId::NONE) {
                        const auto &a_ref = artifacts_info.at(q_ref.reward_artifact_idx);
                        ItemEntity item;
                        auto k_idx = lookup_baseitem_id(a_ref.bi_key);
                        item.prep(k_idx);
                        item.fixed_artifact_idx = q_ref.reward_artifact_idx;
                        item.ident = IDENT_STORE;
                        describe_flavor(player_ptr, name, &item, OD_NAME_ONLY);
                    }

                    sprintf(note, _("\n   - %sを見つけ出す。", "\n   - Find %s."), name);
                    break;
                case QuestKindType::FIND_EXIT:
                    sprintf(note, _(" - 出口に到達する。", " - Reach exit."));
                    break;
                case QuestKindType::KILL_NUMBER:
#ifdef JP
                    sprintf(note, " - %d 体のモンスターを倒す。(あと %d 体)", (int)q_ref.max_num, (int)(q_ref.max_num - q_ref.cur_num));
#else
                    sprintf(note, " - Kill %d monsters, have killed %d.", (int)q_ref.max_num, (int)q_ref.cur_num);
#endif
                    break;

                case QuestKindType::KILL_ALL:
                case QuestKindType::TOWER:
                    sprintf(note, _(" - 全てのモンスターを倒す。", " - Kill all monsters."));
                    break;
                default:
                    break;
                }
            }

            sprintf(tmp_str, _("  %s (危険度:%d階相当)%s\n", "  %s (Danger level: %d)%s\n"), q_ref.name, (int)q_ref.level, note);
            fputs(tmp_str, fff);
            if (q_ref.status == QuestStatusType::COMPLETED) {
                sprintf(tmp_str, _("    クエスト達成 - まだ報酬を受けとってない。\n", "    Quest Completed - Unrewarded\n"));
                fputs(tmp_str, fff);
                continue;
            }

            int k = 0;
            while (quest_text[k][0] && k < 10) {
                fprintf(fff, "    %s\n", quest_text[k]);
                k++;
            }

            continue;
        }

        if (q_ref.level >= rand_level) {
            continue;
        }
        rand_level = q_ref.level;
        if (max_dlv[DUNGEON_ANGBAND] < rand_level) {
            continue;
        }

        r_ptr = &monraces_info[q_ref.r_idx];
        strcpy(name, r_ptr->name.data());
        if (q_ref.max_num <= 1) {
            sprintf(rand_tmp_str, _("  %s (%d 階) - %sを倒す。\n", "  %s (Dungeon level: %d)\n  Kill %s.\n"), q_ref.name, (int)q_ref.level, name);
            continue;
        }

#ifdef JP
        sprintf(rand_tmp_str, "  %s (%d 階) - %d 体の%sを倒す。(あと %d 体)\n", q_ref.name, (int)q_ref.level, (int)q_ref.max_num, name,
            (int)(q_ref.max_num - q_ref.cur_num));
#else
        plural_aux(name);

        sprintf(rand_tmp_str, "  %s (Dungeon level: %d)\n  Kill %d %s, have killed %d.\n", q_ref.name, (int)q_ref.level, (int)q_ref.max_num, name,
            (int)q_ref.cur_num);
#endif
    }

    if (rand_tmp_str[0]) {
        fputs(rand_tmp_str, fff);
    }

    if (!total) {
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
    }
}

static bool do_cmd_knowledge_quests_aux(PlayerType *player_ptr, FILE *fff, QuestId q_idx)
{
    char tmp_str[120];
    char playtime_str[16];
    const auto &quest_list = QuestList::get_instance();
    const auto &q_ref = quest_list[q_idx];

    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto is_fixed_quest = quest_type::is_fixed(q_idx);
    if (is_fixed_quest) {
        QuestId old_quest = floor_ptr->quest_number;
        floor_ptr->quest_number = q_idx;
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
        floor_ptr->quest_number = old_quest;
        if (q_ref.flags & QUEST_FLAG_SILENT) {
            return false;
        }
    }

    strnfmt(playtime_str, sizeof(playtime_str), "%02d:%02d:%02d", q_ref.comptime / (60 * 60), (q_ref.comptime / 60) % 60, q_ref.comptime % 60);

    auto fputs_name_remain = [fff](const auto &name) {
        for (auto i = 1U; i < name.size(); ++i) {
            fprintf(fff, "  %s\n", name[i].data());
        }
    };

    if (is_fixed_quest || !MonsterRace(q_ref.r_idx).is_valid()) {
        auto name = str_separate(q_ref.name, 35);
        sprintf(tmp_str, _("  %-35s (危険度:%3d階相当) - レベル%2d - %s\n", "  %-35s (Danger  level: %3d) - level %2d - %s\n"), name.front().data(), (int)q_ref.level,
            q_ref.complev, playtime_str);
        fputs(tmp_str, fff);
        fputs_name_remain(name);
        return true;
    }

    auto name = str_separate(monraces_info[q_ref.r_idx].name, 35);
    if (q_ref.complev == 0) {
        sprintf(tmp_str, _("  %-35s (%3d階)            -   不戦勝 - %s\n", "  %-35s (Dungeon level: %3d) - Unearned - %s\n"),
            name.front().data(), (int)q_ref.level, playtime_str);
        fputs(tmp_str, fff);
        fputs_name_remain(name);
        return true;
    }

    sprintf(tmp_str, _("  %-35s (%3d階)            - レベル%2d - %s\n", "  %-35s (Dungeon level: %3d) - level %2d - %s\n"), name.front().data(),
        (int)q_ref.level, q_ref.complev, playtime_str);
    fputs(tmp_str, fff);
    fputs_name_remain(name);
    return true;
}

/*
 * Print all finished quests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff セーブファイル (展開済？)
 * @param quest_numbers 受注したことのあるクエスト群
 */
void do_cmd_knowledge_quests_completed(PlayerType *player_ptr, FILE *fff, const std::vector<QuestId> &quest_numbers)
{
    fprintf(fff, _("《達成したクエスト》\n", "< Completed Quest >\n"));
    int16_t total = 0;
    for (auto &q_idx : quest_numbers) {
        const auto &quest_list = QuestList::get_instance();
        const auto &q_ref = quest_list[q_idx];

        if (q_ref.status == QuestStatusType::FINISHED && do_cmd_knowledge_quests_aux(player_ptr, fff, q_idx)) {
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
 * @param quest_numbers 受注したことのあるクエスト群
 */
void do_cmd_knowledge_quests_failed(PlayerType *player_ptr, FILE *fff, const std::vector<QuestId> &quest_numbers)
{
    fprintf(fff, _("《失敗したクエスト》\n", "< Failed Quest >\n"));
    int16_t total = 0;
    for (auto &q_idx : quest_numbers) {
        const auto &quest_list = QuestList::get_instance();
        const auto &q_ref = quest_list[q_idx];

        if (((q_ref.status == QuestStatusType::FAILED_DONE) || (q_ref.status == QuestStatusType::FAILED)) && do_cmd_knowledge_quests_aux(player_ptr, fff, q_idx)) {
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
    const auto &quest_list = QuestList::get_instance();
    GAME_TEXT tmp_str[120];
    int16_t total = 0;
    for (const auto &[q_idx, q_ref] : quest_list) {
        if (q_ref.flags & QUEST_FLAG_SILENT) {
            continue;
        }

        if ((q_ref.type == QuestKindType::RANDOM) && (q_ref.status == QuestStatusType::TAKEN)) {
            total++;
            sprintf(tmp_str, _("  %s (%d階, %s)\n", "  %s (%d, %s)\n"), q_ref.name, (int)q_ref.level, monraces_info[q_ref.r_idx].name.data());
            fputs(tmp_str, fff);
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

    std::vector<QuestId> quest_numbers;
    const auto &quest_list = QuestList::get_instance();
    for (const auto &[q_idx, q_ref] : quest_list) {
        quest_numbers.push_back(q_idx);
    }
    int dummy;
    ang_sort(player_ptr, quest_numbers.data(), &dummy, quest_numbers.size(), ang_sort_comp_quest_num, ang_sort_swap_quest_num);

    do_cmd_knowledge_quests_current(player_ptr, fff);
    fputc('\n', fff);
    do_cmd_knowledge_quests_completed(player_ptr, fff, quest_numbers);
    fputc('\n', fff);
    do_cmd_knowledge_quests_failed(player_ptr, fff, quest_numbers);
    if (w_ptr->wizard) {
        fputc('\n', fff);
        do_cmd_knowledge_quests_wiz_random(fff);
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("クエスト達成状況", "Quest status"), 0, 0);
    fd_kill(file_name);
}
