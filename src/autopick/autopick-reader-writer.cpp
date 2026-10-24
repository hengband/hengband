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
#include <fmt/format.h>
#include <fstream>
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

/*!
 * @brief Load an autopick preference file
 */
void autopick_load_pref(PlayerType *player_ptr, bool disp_mes)
{
    init_autopick();

    const auto path = search_pickpref_path(player_ptr->base_name);
    if (!path.empty()) {
        const auto pickpref_filename = path.filename().string();
        if (process_autopick_file(player_ptr, pickpref_filename) == 0) {
            if (disp_mes) {
                msg_print(_("{}を読み込みました。", "Loaded '{}'."), pickpref_filename);
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
std::filesystem::path search_pickpref_path(std::string_view player_base_name)
{
    for (const auto filename_mode : { PickType::WITH_PNAME, PickType::DEFAULT }) {
        const auto filename = pickpref_filename(player_base_name, filename_mode);
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
std::string pickpref_filename(std::string_view player_base_name, PickType filename_mode)
{
    static const char namebase[] = _("picktype", "pickpref");

    switch (filename_mode) {
    case PickType::DEFAULT:
        return fmt::format("{}.prf", namebase);

    case PickType::WITH_PNAME:
        return fmt::format("{}-{}.prf", namebase, player_base_name);

    default: {
        const auto msg = fmt::format("The value of argument 'filename_mode' is invalid: {}", enum2i(filename_mode));
        THROW_EXCEPTION(std::invalid_argument, msg);
    }
    }
}

/*!
 * @brief Read whole lines of a file to memory
 * @return 行の配列。ファイルを開けなければ tl::nullopt
 */
static tl::optional<std::vector<std::string>> read_text_lines(std::string_view filename)
{
    const auto path = path_build(ANGBAND_DIR_USER, filename);
    auto ifs = std::ifstream(path);
    if (!ifs) {
        return tl::nullopt;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line)) {
        lines.push_back(std::move(line));
        if (is_greater_autopick_max_line(lines.size())) {
            break;
        }
    }

    return lines;
}

/*!
 * @brief Copy the default autopick file to the user directory
 */
static void prepare_default_pickpref(std::string_view player_base_name)
{
    const std::vector<std::string> messages = { _("あなたは「自動拾いエディタ」を初めて起動しました。", "You have activated the Auto-Picker Editor for the first time."),
        _("自動拾いのユーザー設定ファイルがまだ書かれていないので、", "Since user pref file for autopick is not yet created,"),
        _("基本的な自動拾い設定ファイルをlib/pref/picktype.prfからコピーします。", "the default setting is loaded from lib/pref/pickpref.prf .") };

    const auto filename = pickpref_filename(player_base_name, PickType::DEFAULT);
    for (const auto &message : messages) {
        msg_print(message);
    }

    msg_erase();
    const auto path_user = path_build(ANGBAND_DIR_USER, filename);
    auto *user_fp = angband_fopen(path_user, FileOpenMode::WRITE);
    if (!user_fp) {
        return;
    }

    fmt::println(user_fp, "#***");
    for (const auto &message : messages) {
        fmt::println(user_fp, "#***  {}", message);
    }

    fmt::println(user_fp, "#***\n\n");
    const auto path_pref = path_build(ANGBAND_DIR_PREF, filename);
    auto *pref_fp = angband_fopen(path_pref, FileOpenMode::READ);
    if (!pref_fp) {
        angband_fclose(user_fp);
        return;
    }

    while (true) {
        const auto line_str = angband_fgets(pref_fp, MAX_LINELEN);
        if (!line_str) {
            break;
        }
        fmt::println(user_fp, "{}", *line_str);
    }

    angband_fclose(user_fp);
    angband_fclose(pref_fp);
}

/*!
 * @brief Read an autopick prefence file to memory
 * Prepare default if no user file is found
 * @return ファイル名の種類と行の配列 (空のファイルか、どのファイルも読めなければ空)
 */
std::pair<PickType, std::vector<std::string>> read_pickpref_text_lines(std::string_view player_base_name)
{
    /* Try a filename with player name */
    auto filename_mode = PickType::WITH_PNAME;
    auto filename = pickpref_filename(player_base_name, filename_mode);
    auto lines_list = read_text_lines(filename);
    if (!lines_list) {
        filename_mode = PickType::DEFAULT;
        filename = pickpref_filename(player_base_name, filename_mode);
        lines_list = read_text_lines(filename);
    }

    if (!lines_list) {
        prepare_default_pickpref(player_base_name);
        lines_list = read_text_lines(filename);
    }

    return { filename_mode, std::move(lines_list).value_or(std::vector<std::string>{}) };
}

/*!
 * @brief Write whole lines of memory to a file.
 */
bool write_text_lines(std::string_view filename, const std::vector<std::string> &lines)
{
    const auto path = path_build(ANGBAND_DIR_USER, filename);
    auto ofs = std::ofstream(path);
    if (!ofs) {
        return false;
    }

    for (const auto &line : lines) {
        ofs << line << '\n';
    }

    return true;
}
