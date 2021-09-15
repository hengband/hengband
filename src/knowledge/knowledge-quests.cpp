/*!
 * @brief 既知のクエストを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge-quests.h"
#include "core/show-file.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/game-play-options.h"
#include "info-reader/fixed-map-parser.h"
#include "io-dump/dump-util.h"
#include "locale/english.h"
#include "monster-race/monster-race.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-kind-hook.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/sort.h"

#include <numeric>

/*!
 * @brief Check on the status of an active quest
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_checkquest(player_type *player_ptr)
{
    screen_save();
    do_cmd_knowledge_quests(player_ptr);
    screen_load();
}

/*!
 * @brief Print all active quests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @todo player_typeではなくQUEST_IDXを引数にすべきかもしれない
 */
static void do_cmd_knowledge_quests_current(player_type *player_ptr, FILE *fff)
{
    char tmp_str[1024];
    char rand_tmp_str[512] = "\0";
    GAME_TEXT name[MAX_NLEN];
    monster_race *r_ptr;
    int rand_level = 100;
    int total = 0;

    fprintf(fff, _("《遂行中のクエスト》\n", "< Current Quest >\n"));

    for (QUEST_IDX i = 1; i < max_q_idx; i++) {
        bool is_print = quest[i].status == QUEST_STATUS_TAKEN;
        is_print |= (quest[i].status == QUEST_STATUS_STAGE_COMPLETED) && (quest[i].type == QUEST_TYPE_TOWER);
        is_print |= quest[i].status == QUEST_STATUS_COMPLETED;
        if (!is_print)
            continue;

        QUEST_IDX old_quest = player_ptr->current_floor_ptr->inside_quest;
        for (int j = 0; j < 10; j++)
            quest_text[j][0] = '\0';

        quest_text_line = 0;
        player_ptr->current_floor_ptr->inside_quest = i;
        init_flags = INIT_SHOW_TEXT;
        parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
        player_ptr->current_floor_ptr->inside_quest = old_quest;
        if (quest[i].flags & QUEST_FLAG_SILENT)
            continue;

        total++;
        if (quest[i].type != QUEST_TYPE_RANDOM) {
            char note[512] = "\0";

            if (quest[i].status == QUEST_STATUS_TAKEN || quest[i].status == QUEST_STATUS_STAGE_COMPLETED) {
                switch (quest[i].type) {
                case QUEST_TYPE_KILL_LEVEL:
                case QUEST_TYPE_KILL_ANY_LEVEL:
                    r_ptr = &r_info[quest[i].r_idx];
                    strcpy(name, r_ptr->name.c_str());
                    if (quest[i].max_num > 1) {
#ifdef JP
                        sprintf(note, " - %d 体の%sを倒す。(あと %d 体)", (int)quest[i].max_num, name, (int)(quest[i].max_num - quest[i].cur_num));
#else
                        plural_aux(name);
                        sprintf(note, " - kill %d %s, have killed %d.", (int)quest[i].max_num, name, (int)quest[i].cur_num);
#endif
                    } else
                        sprintf(note, _(" - %sを倒す。", " - kill %s."), name);
                    break;

                case QUEST_TYPE_FIND_ARTIFACT:
                    if (quest[i].k_idx) {
                        artifact_type *a_ptr = &a_info[quest[i].k_idx];
                        object_type forge;
                        object_type *q_ptr = &forge;
                        KIND_OBJECT_IDX k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
                        q_ptr->prep(k_idx);
                        q_ptr->name1 = quest[i].k_idx;
                        q_ptr->ident = IDENT_STORE;
                        describe_flavor(player_ptr, name, q_ptr, OD_NAME_ONLY);
                    }
                    sprintf(note, _("\n   - %sを見つけ出す。", "\n   - Find %s."), name);
                    break;
                case QUEST_TYPE_FIND_EXIT:
                    sprintf(note, _(" - 出口に到達する。", " - Reach exit."));
                    break;

                case QUEST_TYPE_KILL_NUMBER:
#ifdef JP
                    sprintf(note, " - %d 体のモンスターを倒す。(あと %d 体)", (int)quest[i].max_num, (int)(quest[i].max_num - quest[i].cur_num));
#else
                    sprintf(note, " - Kill %d monsters, have killed %d.", (int)quest[i].max_num, (int)quest[i].cur_num);
#endif
                    break;

                case QUEST_TYPE_KILL_ALL:
                case QUEST_TYPE_TOWER:
                    sprintf(note, _(" - 全てのモンスターを倒す。", " - Kill all monsters."));
                    break;
                }
            }

            sprintf(tmp_str, _("  %s (危険度:%d階相当)%s\n", "  %s (Danger level: %d)%s\n"), quest[i].name, (int)quest[i].level, note);
            fputs(tmp_str, fff);
            if (quest[i].status == QUEST_STATUS_COMPLETED) {
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

        if (quest[i].level >= rand_level)
            continue;

        rand_level = quest[i].level;
        if (max_dlv[DUNGEON_ANGBAND] < rand_level)
            continue;

        r_ptr = &r_info[quest[i].r_idx];
        strcpy(name, r_ptr->name.c_str());
        if (quest[i].max_num <= 1) {
            sprintf(rand_tmp_str, _("  %s (%d 階) - %sを倒す。\n", "  %s (Dungeon level: %d)\n  Kill %s.\n"), quest[i].name, (int)quest[i].level, name);
            continue;
        }

#ifdef JP
        sprintf(rand_tmp_str, "  %s (%d 階) - %d 体の%sを倒す。(あと %d 体)\n", quest[i].name, (int)quest[i].level, (int)quest[i].max_num, name,
            (int)(quest[i].max_num - quest[i].cur_num));
#else
        plural_aux(name);

        sprintf(rand_tmp_str, "  %s (Dungeon level: %d)\n  Kill %d %s, have killed %d.\n", quest[i].name, (int)quest[i].level, (int)quest[i].max_num, name,
            (int)quest[i].cur_num);
#endif
    }

    if (rand_tmp_str[0])
        fputs(rand_tmp_str, fff);

    if (!total)
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
}

static bool do_cmd_knowledge_quests_aux(player_type *player_ptr, FILE *fff, IDX q_idx)
{
    char tmp_str[120];
    char playtime_str[16];
    quest_type *const q_ptr = &quest[q_idx];

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (is_fixed_quest_idx(q_idx)) {
        IDX old_quest = floor_ptr->inside_quest;
        floor_ptr->inside_quest = q_idx;
        init_flags = INIT_NAME_ONLY;
        parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
        floor_ptr->inside_quest = old_quest;
        if (q_ptr->flags & QUEST_FLAG_SILENT)
            return false;
    }

    strnfmt(playtime_str, sizeof(playtime_str), "%02d:%02d:%02d", q_ptr->comptime / (60 * 60), (q_ptr->comptime / 60) % 60, q_ptr->comptime % 60);

    if (is_fixed_quest_idx(q_idx) || (q_ptr->r_idx == 0)) {
        sprintf(tmp_str, _("  %-35s (危険度:%3d階相当) - レベル%2d - %s\n", "  %-35s (Danger  level: %3d) - level %2d - %s\n"), q_ptr->name, (int)q_ptr->level,
            q_ptr->complev, playtime_str);
        fputs(tmp_str, fff);
        return true;
    }

    if (q_ptr->complev == 0) {
        sprintf(tmp_str, _("  %-35s (%3d階)            -   不戦勝 - %s\n", "  %-35s (Dungeon level: %3d) - Unearned - %s\n"),
            r_info[q_ptr->r_idx].name.c_str(), (int)q_ptr->level, playtime_str);
        fputs(tmp_str, fff);
        return true;
    }

    sprintf(tmp_str, _("  %-35s (%3d階)            - レベル%2d - %s\n", "  %-35s (Dungeon level: %3d) - level %2d - %s\n"), r_info[q_ptr->r_idx].name.c_str(),
        (int)q_ptr->level, q_ptr->complev, playtime_str);
    fputs(tmp_str, fff);
    return true;
}

/*
 * Print all finished quests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff セーブファイル (展開済？)
 * @param quest_num[] 受注したことのあるクエスト群
 */
void do_cmd_knowledge_quests_completed(player_type *player_ptr, FILE *fff, QUEST_IDX quest_num[])
{
    fprintf(fff, _("《達成したクエスト》\n", "< Completed Quest >\n"));
    QUEST_IDX total = 0;
    for (QUEST_IDX i = 1; i < max_q_idx; i++) {
        QUEST_IDX q_idx = quest_num[i];
        quest_type *const q_ptr = &quest[q_idx];

        if (q_ptr->status == QUEST_STATUS_FINISHED && do_cmd_knowledge_quests_aux(player_ptr, fff, q_idx)) {
            ++total;
        }
    }

    if (!total)
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
}

/*
 * Print all failed quests
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff セーブファイル (展開済？)
 * @param quest_num[] 受注したことのあるクエスト群
 */
void do_cmd_knowledge_quests_failed(player_type *player_ptr, FILE *fff, QUEST_IDX quest_num[])
{
    fprintf(fff, _("《失敗したクエスト》\n", "< Failed Quest >\n"));
    QUEST_IDX total = 0;
    for (QUEST_IDX i = 1; i < max_q_idx; i++) {
        QUEST_IDX q_idx = quest_num[i];
        quest_type *const q_ptr = &quest[q_idx];

        if (((q_ptr->status == QUEST_STATUS_FAILED_DONE) || (q_ptr->status == QUEST_STATUS_FAILED)) && do_cmd_knowledge_quests_aux(player_ptr, fff, q_idx)) {
            ++total;
        }
    }

    if (!total)
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
}

/*
 * Print all random quests
 */
static void do_cmd_knowledge_quests_wiz_random(FILE *fff)
{
    fprintf(fff, _("《残りのランダムクエスト》\n", "< Remaining Random Quest >\n"));
    GAME_TEXT tmp_str[120];
    QUEST_IDX total = 0;
    for (QUEST_IDX i = 1; i < max_q_idx; i++) {
        if (quest[i].flags & QUEST_FLAG_SILENT)
            continue;

        if ((quest[i].type == QUEST_TYPE_RANDOM) && (quest[i].status == QUEST_STATUS_TAKEN)) {
            total++;
            sprintf(tmp_str, _("  %s (%d階, %s)\n", "  %s (%d, %s)\n"), quest[i].name, (int)quest[i].level, r_info[quest[i].r_idx].name.c_str());
            fputs(tmp_str, fff);
        }
    }

    if (!total)
        fprintf(fff, _("  なし\n", "  Nothing.\n"));
}

/*
 * Print quest status of all active quests
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_knowledge_quests(player_type *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    std::vector<QUEST_IDX> quest_num(max_q_idx);
    std::iota(quest_num.begin(), quest_num.end(), static_cast<QUEST_IDX>(0));

    int dummy;
    ang_sort(player_ptr, quest_num.data(), &dummy, quest_num.size(), ang_sort_comp_quest_num, ang_sort_swap_quest_num);

    do_cmd_knowledge_quests_current(player_ptr, fff);
    fputc('\n', fff);
    do_cmd_knowledge_quests_completed(player_ptr, fff, quest_num.data());
    fputc('\n', fff);
    do_cmd_knowledge_quests_failed(player_ptr, fff, quest_num.data());
    if (allow_debug_options) {
        fputc('\n', fff);
        do_cmd_knowledge_quests_wiz_random(fff);
    }

    angband_fclose(fff);
    (void)show_file(player_ptr, true, file_name, _("クエスト達成状況", "Quest status"), 0, 0);
    fd_kill(file_name);
}
