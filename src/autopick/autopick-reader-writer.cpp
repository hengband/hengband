#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-initializer.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-util.h"
#include "io/files-util.h"
#include "io/read-pref-file.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief Load an autopick preference file
 */
void autopick_load_pref(player_type *player_ptr, bool disp_mes)
{
    GAME_TEXT buf[80];
    init_autopick();
    angband_strcpy(buf, pickpref_filename(player_ptr, PT_WITH_PNAME), sizeof(buf));
    errr err = process_autopick_file(player_ptr, buf);
    if (err == 0 && disp_mes) {
        msg_format(_("%sを読み込みました。", "Loaded '%s'."), buf);
    }

    if (err < 0) {
        angband_strcpy(buf, pickpref_filename(player_ptr, PT_DEFAULT), sizeof(buf));
        err = process_autopick_file(player_ptr, buf);
        if (err == 0 && disp_mes) {
            msg_format(_("%sを読み込みました。", "Loaded '%s'."), buf);
        }
    }

    if (err && disp_mes) {
        msg_print(_("自動拾い設定ファイルの読み込みに失敗しました。", "Failed to reload autopick preference."));
    }
}

/*!
 * @brief Get file name for autopick preference
 */
concptr pickpref_filename(player_type *player_ptr, int filename_mode)
{
    static const char namebase[] = _("picktype", "pickpref");

    switch (filename_mode) {
    case PT_DEFAULT:
        return format("%s.prf", namebase);

    case PT_WITH_PNAME:
        return format("%s-%s.prf", namebase, player_ptr->base_name);

    default:
        return nullptr;
    }
}

/*!
 * @brief Read whole lines of a file to memory
 */
static std::vector<concptr> read_text_lines(concptr filename)
{
    FILE *fff;

    int lines = 0;
    char buf[1024];

    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, filename);
    fff = angband_fopen(buf, "r");
    if (!fff)
        return {};

    std::vector<concptr> lines_list(MAX_LINES);
    while (angband_fgets(fff, buf, sizeof(buf)) == 0) {
        lines_list[lines++] = string_make(buf);
        if (is_greater_autopick_max_line(lines))
            break;
    }

    if (lines == 0)
        lines_list[0] = string_make("");

    angband_fclose(fff);
    return lines_list;
}

/*!
 * @brief Copy the default autopick file to the user directory
 */
static void prepare_default_pickpref(player_type *player_ptr)
{
    const concptr messages[] = { _("あなたは「自動拾いエディタ」を初めて起動しました。", "You have activated the Auto-Picker Editor for the first time."),
        _("自動拾いのユーザー設定ファイルがまだ書かれていないので、", "Since user pref file for autopick is not yet created,"),
        _("基本的な自動拾い設定ファイルをlib/pref/picktype.prfからコピーします。", "the default setting is loaded from lib/pref/pickpref.prf ."), nullptr };

    concptr filename = pickpref_filename(player_ptr, PT_DEFAULT);
    for (int i = 0; messages[i]; i++) {
        msg_print(messages[i]);
    }

    msg_print(nullptr);
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, filename);
    FILE *user_fp;
    user_fp = angband_fopen(buf, "w");
    if (!user_fp)
        return;

    fprintf(user_fp, "#***\n");
    for (int i = 0; messages[i]; i++) {
        fprintf(user_fp, "#***  %s\n", messages[i]);
    }

    fprintf(user_fp, "#***\n\n\n");
    path_build(buf, sizeof(buf), ANGBAND_DIR_PREF, filename);
    FILE *pref_fp;
    pref_fp = angband_fopen(buf, "r");

    if (!pref_fp) {
        angband_fclose(user_fp);
        return;
    }

    while (!angband_fgets(pref_fp, buf, sizeof(buf))) {
        fprintf(user_fp, "%s\n", buf);
    }

    angband_fclose(user_fp);
    angband_fclose(pref_fp);
}

/*!
 * @brief Read an autopick prefence file to memory
 * Prepare default if no user file is found
 */
std::vector<concptr> read_pickpref_text_lines(player_type *player_ptr, int *filename_mode_p)
{
    /* Try a filename with player name */
    *filename_mode_p = PT_WITH_PNAME;
    char buf[1024];
    strcpy(buf, pickpref_filename(player_ptr, *filename_mode_p));
    std::vector<concptr> lines_list = read_text_lines(buf);

    if (lines_list.empty()) {
        *filename_mode_p = PT_DEFAULT;
        strcpy(buf, pickpref_filename(player_ptr, *filename_mode_p));
        lines_list = read_text_lines(buf);
    }

    if (lines_list.empty()) {
        prepare_default_pickpref(player_ptr);
        lines_list = read_text_lines(buf);
    }

    if (lines_list.empty()) {
        lines_list.resize(MAX_LINES);
        lines_list[0] = string_make("");
    }

    return lines_list;
}

/*!
 * @brief Write whole lines of memory to a file.
 */
bool write_text_lines(concptr filename, concptr *lines_list)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, filename);
    FILE *fff;
    fff = angband_fopen(buf, "w");
    if (!fff)
        return false;

    for (int lines = 0; lines_list[lines]; lines++) {
        angband_fputs(fff, lines_list[lines], 1024);
    }

    angband_fclose(fff);
    return true;
}
