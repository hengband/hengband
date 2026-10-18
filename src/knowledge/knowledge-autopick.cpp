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
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include <fmt/format.h>

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
void do_cmd_knowledge_autopick(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    if (autopick_list.empty()) {
        fmt::print(fff, _("自動破壊/拾いには何も登録されていません。", "No preference for auto picker/destroyer."));
    } else {
        fmt::println(fff, _("   自動拾い/破壊には現在 {}行登録されています。\n", "   There are {} registered lines for auto picker/destroyer.\n"), static_cast<int>(autopick_list.size()));
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

        const auto command_str = entry.action.has(AutopickMethod::DISPLAY) ? fmt::format("[{}]", command) : fmt::format("({})", command);
        fmt::print(fff, "{:>11}", fmt::bytes(command_str));
        const auto line = autopick_line_from_entry(entry);
        fmt::println(fff, " {}", line);
    }

    angband_fclose(fff);

    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("自動拾い/破壊 設定リスト", "Auto-picker/Destroyer"));
    fd_kill(file_name);
}
