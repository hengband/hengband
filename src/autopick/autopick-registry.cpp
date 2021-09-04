/*!
 * @brief 自動拾いにアイテムを登録する
 * @date 2020/04/26
 * @author Hourier
 */

#include "autopick/autopick-registry.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-util.h"
#include "core/asking-player.h"
#include "flavor/flavor-describer.h"
#include "io/files-util.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/special-object-flags.h"
#include "perception/object-perception.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"

static const char autoregister_header[] = "?:$AUTOREGISTER";

/*!
 * @brief Clear auto registered lines in the picktype.prf .
 */
static bool clear_auto_register(player_type *player_ptr)
{
    char pref_file[1024];
    path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_WITH_PNAME));
    FILE *pref_fff;
    pref_fff = angband_fopen(pref_file, "r");

    if (!pref_fff) {
        path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_DEFAULT));
        pref_fff = angband_fopen(pref_file, "r");
    }

    if (!pref_fff) {
        return true;
    }

    char tmp_file[1024];
    FILE *tmp_fff;
    tmp_fff = angband_fopen_temp(tmp_file, sizeof(tmp_file));
    if (!tmp_fff) {
        fclose(pref_fff);
        msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), tmp_file);
        msg_print(NULL);
        return false;
    }

    bool autoregister = false;
    int num = 0;
    char buf[1024];
    while (true) {
        if (angband_fgets(pref_fff, buf, sizeof(buf)))
            break;

        if (autoregister) {
            if (buf[0] != '#' && buf[0] != '?')
                num++;
            continue;
        }

        if (streq(buf, autoregister_header)) {
            autoregister = true;
        } else {
            fprintf(tmp_fff, "%s\n", buf);
        }
    }

    angband_fclose(pref_fff);
    angband_fclose(tmp_fff);

    bool okay = true;
    if (num) {
        msg_format(_("以前のキャラクター用の自動設定(%d行)が残っています。", "Auto registered lines (%d lines) for previous character are remaining."), num);
        strcpy(buf, _("古い設定行は削除します。よろしいですか？", "These lines will be deleted.  Are you sure? "));

        if (!get_check(buf)) {
            okay = false;
            autoregister = false;

            msg_print(_("エディタのカット&ペースト等を使って必要な行を避難してください。", "Use cut & paste of auto picker editor (_) to keep old prefs."));
        }
    }

    if (autoregister) {
        tmp_fff = angband_fopen(tmp_file, "r");
        pref_fff = angband_fopen(pref_file, "w");

        while (!angband_fgets(tmp_fff, buf, sizeof(buf)))
            fprintf(pref_fff, "%s\n", buf);

        angband_fclose(pref_fff);
        angband_fclose(tmp_fff);
    }

    fd_kill(tmp_file);
    return okay;
}

/*!
 * @brief Automatically register an auto-destroy preference line
 */
bool autopick_autoregister(player_type *player_ptr, object_type *o_ptr)
{
    autopick_type an_entry, *entry = &an_entry;
    int autopick_registered = find_autopick_list(player_ptr, o_ptr);
    if (autopick_registered != -1) {
        concptr what;
        byte act = autopick_list[autopick_registered].action;
        if (act & DO_AUTOPICK)
            what = _("自動で拾う", "auto-pickup");
        else if (act & DO_AUTODESTROY)
            what = _("自動破壊する", "auto-destroy");
        else if (act & DONT_AUTOPICK)
            what = _("放置する", "leave on floor");
        else
            what = _("確認して拾う", "query auto-pickup");

        msg_format(_("そのアイテムは既に%sように設定されています。", "The object is already registered to %s."), what);
        return false;
    }

    if ((o_ptr->is_known() && o_ptr->is_artifact()) || ((o_ptr->ident & IDENT_SENSE) && (o_ptr->feeling == FEEL_TERRIBLE || o_ptr->feeling == FEEL_SPECIAL))) {
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        msg_format(_("%sは破壊不能だ。", "You cannot auto-destroy %s."), o_name);
        return false;
    }

    if (!player_ptr->autopick_autoregister) {
        if (!clear_auto_register(player_ptr))
            return false;
    }

    char buf[1024];
    char pref_file[1024];
    FILE *pref_fff;
    path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_WITH_PNAME));
    pref_fff = angband_fopen(pref_file, "r");

    if (!pref_fff) {
        path_build(pref_file, sizeof(pref_file), ANGBAND_DIR_USER, pickpref_filename(player_ptr, PT_DEFAULT));
        pref_fff = angband_fopen(pref_file, "r");
    }

    if (pref_fff) {
        while (true) {
            if (angband_fgets(pref_fff, buf, sizeof(buf))) {
                player_ptr->autopick_autoregister = false;
                break;
            }

            if (streq(buf, autoregister_header)) {
                player_ptr->autopick_autoregister = true;
                break;
            }
        }

        fclose(pref_fff);
    } else {
        /*
         * File could not be opened for reading.  Assume header not
         * present.
         */
        player_ptr->autopick_autoregister = false;
    }

    pref_fff = angband_fopen(pref_file, "a");
    if (!pref_fff) {
        msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), pref_file);
        msg_print(NULL);
        return false;
    }

    if (!player_ptr->autopick_autoregister) {
        fprintf(pref_fff, "%s\n", autoregister_header);

        fprintf(pref_fff, "%s\n", _("# *警告!!* 以降の行は自動登録されたものです。", "# *Warning!* The lines below will be deleted later."));
        fprintf(pref_fff, "%s\n",
            _("# 後で自動的に削除されますので、必要な行は上の方へ移動しておいてください。",
                "# Keep it by cut & paste if you need these lines for future characters."));
        player_ptr->autopick_autoregister = true;
    }

    autopick_entry_from_object(player_ptr, entry, o_ptr);
    entry->action = DO_AUTODESTROY;
    add_autopick_list(entry);

    concptr tmp = autopick_line_from_entry(entry);
    fprintf(pref_fff, "%s\n", tmp);
    string_free(tmp);
    fclose(pref_fff);
    return true;
}
