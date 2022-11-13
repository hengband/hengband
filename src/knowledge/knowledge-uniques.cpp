/*!
 * @brief 既知/存命のユニークを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-uniques.h"
#include "core/show-file.h"
#include "game-option/cheat-options.h"
#include "io-dump/dump-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/sort.h"
#include "util/string-processor.h"

struct unique_list_type {
    bool is_alive;
    uint16_t why;
    std::vector<MonsterRaceId> who;
    int num_uniques[10];
    int num_uniques_surface;
    int num_uniques_over100;
    int num_uniques_total;
    int max_lev;
};

unique_list_type *initialize_unique_lsit_type(unique_list_type *unique_list_ptr, bool is_alive)
{
    unique_list_ptr->is_alive = is_alive;
    unique_list_ptr->why = 2;
    unique_list_ptr->num_uniques_surface = 0;
    unique_list_ptr->num_uniques_over100 = 0;
    unique_list_ptr->num_uniques_total = 0;
    unique_list_ptr->max_lev = -1;
    for (IDX i = 0; i < 10; i++) {
        unique_list_ptr->num_uniques[i] = 0;
    }

    return unique_list_ptr;
}

/*!
 * @brief モンスターリストを走査し、生きているか死んでいるユニークだけを抽出する
 * @param r_ptr モンスター種別への参照ポインタ
 * @param is_alive 生きているユニークのリストならばTRUE、撃破したユニークのリストならばFALSE
 * @return is_aliveの条件に見合うユニークがいたらTRUE、それ以外はFALSE
 * @details 闘技場のモンスターとは再戦できないので、生きているなら表示から外す
 */
static bool sweep_uniques(monster_race *r_ptr, bool is_alive)
{
    if (r_ptr->name.empty()) {
        return false;
    }

    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {

        return false;
    }

    if (!cheat_know && !r_ptr->r_sights) {
        return false;
    }

    bool is_except_arena = is_alive ? (r_ptr->rarity > 100) && ((r_ptr->flags1 & RF1_QUESTOR) == 0) : false;
    if (!r_ptr->rarity || is_except_arena) {
        return false;
    }

    if (is_alive) {
        if (r_ptr->max_num == 0) {
            return false;
        }
    } else {
        if (r_ptr->max_num > 0) {
            return false;
        }
    }

    return true;
}

static void display_uniques(unique_list_type *unique_list_ptr, FILE *fff)
{
    if (unique_list_ptr->num_uniques_surface) {
        concptr surface_desc = unique_list_ptr->is_alive ? _("     地上  生存: %3d体\n", "      Surface  alive: %3d\n")
                                                         : _("     地上  撃破: %3d体\n", "      Surface  dead: %3d\n");
        fprintf(fff, surface_desc, unique_list_ptr->num_uniques_surface);
        unique_list_ptr->num_uniques_total += unique_list_ptr->num_uniques_surface;
    }

    for (IDX i = 0; i <= unique_list_ptr->max_lev; i++) {
        concptr dungeon_desc = unique_list_ptr->is_alive ? _("%3d-%3d階  生存: %3d体\n", "Level %3d-%3d  alive: %3d\n")
                                                         : _("%3d-%3d階  撃破: %3d体\n", "Level %3d-%3d  dead: %3d\n");
        fprintf(fff, dungeon_desc, 1 + i * 10, 10 + i * 10, unique_list_ptr->num_uniques[i]);
        unique_list_ptr->num_uniques_total += unique_list_ptr->num_uniques[i];
    }

    if (unique_list_ptr->num_uniques_over100) {
        concptr deep_desc = unique_list_ptr->is_alive ? _("101-   階  生存: %3d体\n", "Level 101-     alive: %3d\n")
                                                      : _("101-   階  撃破: %3d体\n", "Level 101-     dead: %3d\n");
        fprintf(fff, deep_desc, unique_list_ptr->num_uniques_over100);
        unique_list_ptr->num_uniques_total += unique_list_ptr->num_uniques_over100;
    }

    if (unique_list_ptr->num_uniques_total) {
        fputs(_("---------  -----------\n", "-------------  ----------\n"), fff);
        concptr total_desc = unique_list_ptr->is_alive ? _("     合計  生存: %3d体\n\n", "        Total  alive: %3d\n\n")
                                                       : _("     合計  撃破: %3d体\n\n", "        Total  dead: %3d\n\n");
        fprintf(fff, total_desc, unique_list_ptr->num_uniques_total);
    } else {
        concptr no_unique_desc = unique_list_ptr->is_alive ? _("現在は既知の生存ユニークはいません。\n", "No known uniques alive.\n")
                                                           : _("現在は既知の撃破ユニークはいません。\n", "No known uniques dead.\n");
        fputs(no_unique_desc, fff);
    }

    char buf[80];
    for (auto r_idx : unique_list_ptr->who) {
        auto *r_ptr = &monraces_info[r_idx];

        if (r_ptr->defeat_level && r_ptr->defeat_time) {
            sprintf(buf, _(" - レベル%2d - %d:%02d:%02d", " - level %2d - %d:%02d:%02d"), r_ptr->defeat_level, r_ptr->defeat_time / (60 * 60),
                (r_ptr->defeat_time / 60) % 60, r_ptr->defeat_time % 60);
        } else {
            buf[0] = '\0';
        }

        const auto name = str_separate(r_ptr->name, 40);
        fprintf(fff, _("     %-40s (レベル%3d)%s\n", "     %-40s (level %3d)%s\n"), name.front().data(), (int)r_ptr->level, buf);
        for (auto i = 1U; i < name.size(); ++i) {
            fprintf(fff, "     %s\n", name[i].data());
        }
    }
}

/*!
 * @brief 既知の生きているユニークまたは撃破済ユニークの一覧を表示させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param is_alive 生きているユニークのリストならばTRUE、撃破したユニークのリストならばFALSE
 */
void do_cmd_knowledge_uniques(PlayerType *player_ptr, bool is_alive)
{
    unique_list_type tmp_list;
    unique_list_type *unique_list_ptr = initialize_unique_lsit_type(&tmp_list, is_alive);
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    for (auto &[r_idx, r_ref] : monraces_info) {
        if (!MonsterRace(r_ref.idx).is_valid()) {
            continue;
        }
        if (!sweep_uniques(&r_ref, unique_list_ptr->is_alive)) {
            continue;
        }

        if (r_ref.level) {
            int lev = (r_ref.level - 1) / 10;
            if (lev < 10) {
                unique_list_ptr->num_uniques[lev]++;
                if (unique_list_ptr->max_lev < lev) {
                    unique_list_ptr->max_lev = lev;
                }
            } else {
                unique_list_ptr->num_uniques_over100++;
            }
        } else {
            unique_list_ptr->num_uniques_surface++;
        }

        unique_list_ptr->who.push_back(r_ref.idx);
    }

    ang_sort(player_ptr, unique_list_ptr->who.data(), &unique_list_ptr->why, unique_list_ptr->who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    display_uniques(unique_list_ptr, fff);
    angband_fclose(fff);
    concptr title_desc = unique_list_ptr->is_alive ? _("まだ生きているユニーク・モンスター", "Alive Uniques") : _("もう撃破したユニーク・モンスター", "Dead Uniques");
    (void)show_file(player_ptr, true, file_name, title_desc, 0, 0);
    fd_kill(file_name);
}
