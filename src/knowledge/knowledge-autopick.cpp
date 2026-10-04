/*!
 * @brief 自動拾いの登録状況を表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-autopick.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-util.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "io-dump/dump-util.h"
#include "io/temp-file.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include <fmt/format.h>
#include <vector>

/*!
 * @brief 自動拾い設定ファイルをロードするコマンドのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_reload_autopick(PlayerType *player_ptr)
{
    if (!input_check(_("自動拾い設定ファイルをロードしますか? ", "Reload auto-pick preference file? "))) {
        return;
    }

    autopick_load_pref(player_ptr, true);
}

/*
 * Check the status of "autopick"
 */
tl::optional<std::string> do_cmd_knowledge_autopick(PlayerType *player_ptr)
{
    TempFile tf;
    if (const auto &error_message = tf.get_error_message(); error_message) {
        return *error_message;
    }

    std::vector<std::string> lines;
    if (autopick_list.empty()) {
        lines.push_back(_("自動破壊/拾いには何も登録されていません。", "No preference for auto picker/destroyer."));
    } else {
        constexpr auto fmt = _("   自動拾い/破壊には現在 {}行登録されています。\n", "   There are {} registered lines for auto picker/destroyer.\n");
        lines.push_back(fmt::format(fmt, autopick_list.size()));
    }

    for (const auto &entry : autopick_list) {
        std::string command;
        if (entry.action.has(AutopickMethod::NOT_AUTOPICK)) {
            command = _("放置", "Leave");
        } else if (entry.action.has(AutopickMethod::AUTODESTROY)) {
            command = _("破壊", "Destroy");
        } else if (entry.action.has(AutopickMethod::AUTOPICK)) {
            command = _("拾う", "Pickup");
        } else {
            command = _("確認", "Query");
        }

        const auto fmt = entry.action.has(AutopickMethod::DISPLAY) ? "[%s]" : "(%s)";
        const auto line = autopick_line_from_entry(entry);
        lines.push_back(fmt::format("{:11} {}", format(fmt, command.data()), line));
    }

    tf.write_lines(lines);
    if (const auto &error_message = tf.get_error_message(); error_message) {
        return *error_message;
    }

    FileDisplayer(player_ptr->name).display(true, tf.get_path().string(), 0, 0, _("自動拾い/破壊 設定リスト", "Auto-picker/Destroyer"));
    return tl::nullopt;
}
