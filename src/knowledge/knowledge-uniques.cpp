/*!
 * @brief 既知/存命のユニークを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-uniques.h"
#include "core/show-file.h"
#include "game-option/cheat-options.h"
#include "io-dump/dump-util.h"
#include "io/temp-file.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-records.h"
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
    const auto &monraces = MonraceList::get_instance();
    const auto &records = MonraceRecords::get_instance();
    for (auto &[monrace_id, monrace] : monraces) {
        if (!cheat_know && !records.has_been_seen(monrace_id)) {
            continue;
        }

        if (!monrace->is_valid() || !monrace->should_display(this->is_alive)) {
            continue;
        }

        if (!monrace->level) {
            this->num_uniques_surface++;
            this->monrace_ids.push_back(monrace_id);
            continue;
        }

        const auto lev = (monrace->level - 1) / 10;
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

static std::vector<std::string> build_unique_names(UniqueList &unique_list)
{
    std::vector<std::string> output_lines;
    if (unique_list.num_uniques_surface) {
        const auto surface_desc = unique_list.is_alive
                                      ? format(_("     地上  生存: %3d体", "      Surface  alive: %3d"), unique_list.num_uniques_surface)
                                      : format(_("     地上  撃破: %3d体", "      Surface  dead: %3d"), unique_list.num_uniques_surface);
        output_lines.push_back(surface_desc);
        unique_list.num_uniques_total += unique_list.num_uniques_surface;
    }

    for (auto i = 0; i <= unique_list.max_lev; i++) {
        const auto dungeon_desc = unique_list.is_alive
                                      ? format(_("%3d-%3d階  生存: %3d体", "Level %3d-%3d  alive: %3d"), 1 + i * 10, 10 + i * 10, unique_list.num_uniques[i])
                                      : format(_("%3d-%3d階  撃破: %3d体", "Level %3d-%3d  dead: %3d"), 1 + i * 10, 10 + i * 10, unique_list.num_uniques[i]);
        output_lines.push_back(dungeon_desc);
        unique_list.num_uniques_total += unique_list.num_uniques[i];
    }

    if (unique_list.num_uniques_over100) {
        const auto deep_desc = unique_list.is_alive
                                   ? format(_("101-   階  生存: %3d体", "Level 101-     alive: %3d"), unique_list.num_uniques_over100)
                                   : format(_("101-   階  撃破: %3d体", "Level 101-     dead: %3d"), unique_list.num_uniques_over100);
        output_lines.push_back(deep_desc);
        unique_list.num_uniques_total += unique_list.num_uniques_over100;
    }

    if (unique_list.num_uniques_total) {
        output_lines.emplace_back(_("---------  -----------", "-------------  ----------"));
        const auto total_desc = unique_list.is_alive
                                    ? format(_("     合計  生存: %3d体", "        Total  alive: %3d"), unique_list.num_uniques_total)
                                    : format(_("     合計  撃破: %3d体", "        Total  dead: %3d"), unique_list.num_uniques_total);
        output_lines.push_back(total_desc);
        output_lines.emplace_back("");
    } else {
        output_lines.emplace_back(unique_list.is_alive ? _("現在は既知の生存ユニークはいません。", "No known uniques alive.")
                                                       : _("現在は既知の撃破ユニークはいません。", "No known uniques dead."));
    }

    const auto &monraces = MonraceList::get_instance();
    for (auto monrace_id : unique_list.monrace_ids) {
        const auto &monrace = monraces.get_monrace(monrace_id);
        std::string details;
        if (!unique_list.is_alive && (monrace.defeat_level > 0) && (monrace.defeat_time > 0)) {
            details = format(_(" - レベル%2d - %d:%02d:%02d", " - level %2d - %d:%02d:%02d"), monrace.defeat_level, monrace.defeat_time / (60 * 60),
                (monrace.defeat_time / 60) % 60, monrace.defeat_time % 60);
        }

        const auto name = str_separate(monrace.name, 40);
        output_lines.push_back(format(_("     %-40s (レベル%3d)%s", "     %-40s (level %3d)%s"), name.front().data(), static_cast<int>(monrace.level), details.data()));
        for (auto i = 1U; i < name.size(); ++i) {
            output_lines.push_back(format("     %s", name[i].data()));
        }
    }

    return output_lines;
}

/*!
 * @brief 既知の生きているユニークまたは撃破済ユニークの一覧を表示させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param is_alive 生きているユニークのリストならばTRUE、撃破したユニークのリストならばFALSE
 * @return エラーが発生した場合はエラーメッセージ、正常終了時はtl::nullopt
 */
tl::optional<std::string> do_cmd_knowledge_uniques(PlayerType *player_ptr, bool is_alive)
{
    TempFile tf;
    if (const auto &error_message = tf.get_error_message(); error_message) {
        return *error_message;
    }

    UniqueList unique_list(is_alive);
    unique_list.sweep();
    const auto &monraces = MonraceList::get_instance();
    std::stable_sort(unique_list.monrace_ids.begin(), unique_list.monrace_ids.end(), [&monraces](auto x, auto y) { return monraces.order(x, y); });
    const auto output_lines = build_unique_names(unique_list);
    tf.write_lines(output_lines);
    if (const auto &error_message = tf.get_error_message(); error_message) {
        return *error_message;
    }

    const auto title_desc = unique_list.is_alive ? _("まだ生きているユニーク・モンスター", "Alive Uniques") : _("もう撃破したユニーク・モンスター", "Dead Uniques");
    FileDisplayer(player_ptr->name).display(true, tf.get_path().string(), 0, 0, title_desc);
    return tl::nullopt;
}
