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

/*!
 * @brief 自動拾い設定ファイルをロードするコマンドのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_reload_autopick(PlayerType *player_ptr)
{
    if (!get_check(_("自動拾い設定ファイルをロードしますか? ", "Reload auto-pick preference file? "))) {
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
        fprintf(fff, _("自動破壊/拾いには何も登録されていません。", "No preference for auto picker/destroyer."));
    } else {
        fprintf(fff, _("   自動拾い/破壊には現在 %d行登録されています。\n\n", "   There are %d registered lines for auto picker/destroyer.\n\n"),
            static_cast<int>(autopick_list.size()));
    }

    for (const auto &entry : autopick_list) {
        concptr tmp;
        byte act = entry.action;
        if (act & DONT_AUTOPICK) {
            tmp = _("放置", "Leave");
        } else if (act & DO_AUTODESTROY) {
            tmp = _("破壊", "Destroy");
        } else if (act & DO_AUTOPICK) {
            tmp = _("拾う", "Pickup");
        } else {
            tmp = _("確認", "Query");
        }

        if (act & DO_DISPLAY) {
            fprintf(fff, "%11s", format("[%s]", tmp).data());
        } else {
            fprintf(fff, "%11s", format("(%s)", tmp).data());
        }

        tmp = autopick_line_from_entry(entry);
        fprintf(fff, " %s", tmp);
        string_free(tmp);
        fprintf(fff, "\n");
    }

    angband_fclose(fff);

    (void)show_file(player_ptr, true, file_name, 0, 0, _("自動拾い/破壊 設定リスト", "Auto-picker/Destroyer"));
    fd_kill(file_name);
}
