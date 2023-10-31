#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-initializer.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-util.h"
#include "io/files-util.h"
#include "io/read-pref-file.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <string>
#include <string_view>
#include <vector>

/*!
 * @brief Load an autopick preference file
 */
void autopick_load_pref(PlayerType *player_ptr, bool disp_mes)
{
    init_autopick();

    const auto path = search_pickpref_path(player_ptr);
    if (!path.empty()) {
        const auto pickpref_filename = path.filename().string();
        if (process_autopick_file(player_ptr, pickpref_filename) == 0) {
            if (disp_mes) {
                msg_format(_("%sを読み込みました。", "Loaded '%s'."), pickpref_filename.data());
            }
            return;
        }
    }

    if (disp_mes) {
        msg_print(_("自動拾い設定ファイルの読み込みに失敗しました。", "Failed to reload autopick preference."));
    }
}

/*!
 * @brief 自動拾い設定ファイルのパスを返す
 *
 * ユーザディレクトリを "picktype-プレイヤー名.prf"、"picktype.prf" の順にファイルが存在するかどうかを確認し、
 * 存在した場合はそのパスを返す。どちらも存在しない場合は空のパスを返す。
 *
 * @return 見つかった自動拾い設定ファイルのパス。見つからなかった場合は空のパス。
 */
std::filesystem::path search_pickpref_path(PlayerType *player_ptr)
{
    for (const auto filename_mode : { PT_WITH_PNAME, PT_DEFAULT }) {
        const auto filename = pickpref_filename(player_ptr, filename_mode);
        const auto path = path_build(ANGBAND_DIR_USER, filename);
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return {};
}

/*!
 * @brief Get file name for autopick preference
 */
std::string pickpref_filename(PlayerType *player_ptr, int filename_mode)
{
    static const char namebase[] = _("picktype", "pickpref");

    switch (filename_mode) {
    case PT_DEFAULT:
        return format("%s.prf", namebase);

    case PT_WITH_PNAME:
        return format("%s-%s.prf", namebase, player_ptr->base_name);

    default: {
        const auto msg = format("The value of argument 'filename_mode' is invalid: %d", filename_mode);
        THROW_EXCEPTION(std::invalid_argument, msg);
    }
    }
}

/*!
 * @brief Read whole lines of a file to memory
 */
static std::vector<concptr> read_text_lines(std::string_view filename)
{
    const auto &path = path_build(ANGBAND_DIR_USER, filename);
    auto *fff = angband_fopen(path, FileOpenMode::READ);
    if (!fff) {
        return {};
    }

    auto lines = 0;
    std::vector<concptr> lines_list(MAX_LINES);
    char buf[1024]{};
    while (angband_fgets(fff, buf, sizeof(buf)) == 0) {
        lines_list[lines++] = string_make(buf);
        if (is_greater_autopick_max_line(lines)) {
            break;
        }
    }

    if (lines == 0) {
        lines_list[0] = string_make("");
    }

    angband_fclose(fff);
    return lines_list;
}

/*!
 * @brief Copy the default autopick file to the user directory
 */
static void prepare_default_pickpref(PlayerType *player_ptr)
{
    const std::vector<std::string> messages = { _("あなたは「自動拾いエディタ」を初めて起動しました。", "You have activated the Auto-Picker Editor for the first time."),
        _("自動拾いのユーザー設定ファイルがまだ書かれていないので、", "Since user pref file for autopick is not yet created,"),
        _("基本的な自動拾い設定ファイルをlib/pref/picktype.prfからコピーします。", "the default setting is loaded from lib/pref/pickpref.prf .") };

    const auto filename = pickpref_filename(player_ptr, PT_DEFAULT);
    for (const auto &message : messages) {
        msg_print(message);
    }

    msg_print(nullptr);
    const auto &path_user = path_build(ANGBAND_DIR_USER, filename);
    auto *user_fp = angband_fopen(path_user, FileOpenMode::WRITE);
    if (!user_fp) {
        return;
    }

    fprintf(user_fp, "#***\n");
    for (const auto &message : messages) {
        fprintf(user_fp, "#***  %s\n", message.data());
    }

    fprintf(user_fp, "#***\n\n\n");
    const auto &path_pref = path_build(ANGBAND_DIR_PREF, filename);
    auto *pref_fp = angband_fopen(path_pref, FileOpenMode::READ);
    if (!pref_fp) {
        angband_fclose(user_fp);
        return;
    }

    char buf[1024]{};
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
std::vector<concptr> read_pickpref_text_lines(PlayerType *player_ptr, int *filename_mode_p)
{
    /* Try a filename with player name */
    *filename_mode_p = PT_WITH_PNAME;
    auto filename = pickpref_filename(player_ptr, *filename_mode_p);
    std::vector<concptr> lines_list = read_text_lines(filename);

    if (lines_list.empty()) {
        *filename_mode_p = PT_DEFAULT;
        filename = pickpref_filename(player_ptr, *filename_mode_p);
        lines_list = read_text_lines(filename);
    }

    if (lines_list.empty()) {
        prepare_default_pickpref(player_ptr);
        lines_list = read_text_lines(filename);
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
bool write_text_lines(std::string_view filename, const std::vector<concptr> &lines)
{
    const auto &path = path_build(ANGBAND_DIR_USER, filename);
    auto *fff = angband_fopen(path, FileOpenMode::WRITE);
    if (!fff) {
        return false;
    }

    for (const auto *line : lines) {
        if (line == nullptr) {
            break;
        }

        angband_fputs(fff, line, 1024);
    }

    angband_fclose(fff);
    return true;
}
