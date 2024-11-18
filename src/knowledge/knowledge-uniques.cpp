/*!
 * @brief 既知/存命のユニークを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-uniques.h"
#include "core/show-file.h"
#include "io-dump/dump-util.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/string-processor.h"

class UniqueList {
public:
    UniqueList(bool is_alive);
    int num_uniques[10]{};
    bool is_alive;
    std::vector<MonraceId> monrace_ids{};
    int num_uniques_surface = 0;
    int num_uniques_over100 = 0;
    int num_uniques_total = 0;
    int max_lev = -1;

    void sweep();
};

UniqueList::UniqueList(bool is_alive)
    : is_alive(is_alive)
{
}

void UniqueList::sweep()
{
    auto &monraces = MonraceList::get_instance();
    for (auto &[monrace_id, monrace] : monraces) {
        if (!monrace.is_valid() || !monrace.should_display(this->is_alive)) {
            continue;
        }

        if (!monrace.level) {
            this->num_uniques_surface++;
            this->monrace_ids.push_back(monrace_id);
            continue;
        }

        const auto lev = (monrace.level - 1) / 10;
        if (lev >= 10) {
            this->num_uniques_over100++;
            this->monrace_ids.push_back(monrace_id);
            continue;
        }

        this->num_uniques[lev]++;
        if (this->max_lev < lev) {
            this->max_lev = lev;
        }

        this->monrace_ids.push_back(monrace_id);
    }
}

static void display_uniques(UniqueList *unique_list_ptr, FILE *fff)
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

    const auto &monraces = MonraceList::get_instance();
    for (auto monrace_id : unique_list_ptr->monrace_ids) {
        const auto &monrace = monraces.get_monrace(monrace_id);
        std::string details;
        if (monrace.defeat_level && monrace.defeat_time) {
            details = format(_(" - レベル%2d - %d:%02d:%02d", " - level %2d - %d:%02d:%02d"), monrace.defeat_level, monrace.defeat_time / (60 * 60),
                (monrace.defeat_time / 60) % 60, monrace.defeat_time % 60);
        }

        const auto name = str_separate(monrace.name, 40);
        fprintf(fff, _("     %-40s (レベル%3d)%s\n", "     %-40s (level %3d)%s\n"), name.front().data(), (int)monrace.level, details.data());
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
    UniqueList unique_list(is_alive);
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    unique_list.sweep();
    const auto &monraces = MonraceList::get_instance();
    std::stable_sort(unique_list.monrace_ids.begin(), unique_list.monrace_ids.end(), [&monraces](auto x, auto y) { return monraces.order(x, y); });
    display_uniques(&unique_list, fff);
    angband_fclose(fff);
    concptr title_desc = unique_list.is_alive ? _("まだ生きているユニーク・モンスター", "Alive Uniques") : _("もう撃破したユニーク・モンスター", "Dead Uniques");
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, title_desc);
    fd_kill(file_name);
}
