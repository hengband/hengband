/*
 * @brief プレイヤーのインターフェイスに関するコマンドの実装 / Interface commands
 * @date 2023/04/30
 * @author Mogami & Hourier
 */

#include "io/read-pref-file.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "core/asking-player.h"
#include "io-dump/dump-remover.h"
#include "io/files-util.h"
#include "io/interpret-pref-file.h"
#include "io/pref-file-expressor.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/buffer-shaper.h"
#include "util/finalizer.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <filesystem>
#include <string>

//!< @todo コールバック関数に変更するので、いずれ消す.
#define PREF_TYPE_NORMAL 0
#define PREF_TYPE_AUTOPICK 1
#define PREF_TYPE_HISTPREF 2

char auto_dump_header[] = "# vvvvvvv== %s ==vvvvvvv";
char auto_dump_footer[] = "# ^^^^^^^== %s ==^^^^^^^";

// Mark strings for auto dump

// Variables for auto dump
static int auto_dump_line_num;

/*!
 * @brief process_pref_fileのサブルーチン /
 * Open the "user pref file" and parse it.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 読み込むファイル名
 * @param preftype prefファイルのタイプ
 * @return エラーコード
 * @todo 関数名を変更する
 */
static errr process_pref_file_aux(PlayerType *player_ptr, const std::filesystem::path &name, int preftype)
{
    auto *fp = angband_fopen(name, FileOpenMode::READ);
    if (!fp) {
        return -1;
    }

    int line = -1;
    errr err = 0;
    bool bypass = false;
    std::string error_line;
    while (true) {
        auto line_str = angband_fgets(fp);
        if (!line_str) {
            break;
        }
        line++;
        if (line_str->empty()) {
            continue;
        }

#ifdef JP
        if (!iskanji(line_str->front()))
#endif
            if (iswspace(line_str->front())) {
                continue;
            }

        if (line_str->starts_with('#')) {
            continue;
        }
        error_line = *line_str;

        /* Process "?:<expr>" */
        if (line_str->starts_with("?:")) {
            char f;
            char *s = line_str->data() + 2;
            auto v = process_pref_file_expr(player_ptr, &s, &f);
            bypass = v == "0";
            continue;
        }

        if (bypass) {
            continue;
        }

        /* Process "%:<file>" */
        if (line_str->starts_with("%:")) {
            static int depth_count = 0;
            if (depth_count > 20) {
                continue;
            }

            depth_count++;
            std::string_view file(*line_str);
            file.remove_prefix(2);
            switch (preftype) {
            case PREF_TYPE_AUTOPICK:
                (void)process_autopick_file(player_ptr, file);
                break;
            case PREF_TYPE_HISTPREF:
                (void)process_histpref_file(player_ptr, file);
                break;
            default:
                (void)process_pref_file(player_ptr, file);
                break;
            }

            depth_count--;
            continue;
        }

        err = interpret_pref_file(player_ptr, line_str->data());
        if (err != 0) {
            if (preftype != PREF_TYPE_AUTOPICK) {
                break;
            }

            process_autopick_file_command(line_str->data());
            err = 0;
        }
    }

    if (err != 0) {
        /* Print error message */
        /* ToDo: Add better error messages */
        const auto &name_str = name.string();
        msg_format(_("ファイル'%s'の%d行でエラー番号%dのエラー。", "Error %d in line %d of file '%s'."), _(name_str.data(), err), line, _(err, name_str.data()));
        msg_format(_("('%s'を解析中)", "Parsing '%s'"), error_line.data());
        msg_erase();
    }

    angband_fclose(fp);
    return err;
}

/*!
 * @brief pref設定ファイルを読み込み設定を反映させる /
 * Process the "user pref file" with the given name
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 読み込むファイル名
 * @param only_user_dir trueを指定するとANGBAND_DIR_USERからの読み込みのみ行う
 * @return エラーコード
 * @details
 * <pre>
 * See the functions above for a list of legal "commands".
 * We also accept the special "?" and "%" directives, which
 * allow conditional evaluation and filename inclusion.
 * </pre>
 */
errr process_pref_file(PlayerType *player_ptr, std::string_view name, bool only_user_dir)
{
    errr err1 = 0;
    if (!only_user_dir) {
        const auto path = path_build(ANGBAND_DIR_PREF, name);
        err1 = process_pref_file_aux(player_ptr, path, PREF_TYPE_NORMAL);
        if (err1 > 0) {
            return err1;
        }
    }

    const auto path = path_build(ANGBAND_DIR_USER, name);
    errr err2 = process_pref_file_aux(player_ptr, path, PREF_TYPE_NORMAL);
    if (err2 < 0 && !err1) {
        return -2;
    }

    return err2;
}

/*!
 * @brief 自動拾いファイルを読み込む /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name ファイル名
 * @details
 */
errr process_autopick_file(PlayerType *player_ptr, std::string_view name)
{
    const auto path = path_build(ANGBAND_DIR_USER, name);
    return process_pref_file_aux(player_ptr, path, PREF_TYPE_AUTOPICK);
}

/*!
 * @brief プレイヤーの生い立ちファイルを読み込む /
 * Process file for player's history editor.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name ファイル名
 * @return エラーコード
 * @details
 */
errr process_histpref_file(PlayerType *player_ptr, std::string_view name)
{
    auto &world = AngbandWorld::get_instance();
    const auto old_character_xtra = world.character_xtra;
    const auto path = path_build(ANGBAND_DIR_USER, name);
    world.character_xtra = true;
    errr err = process_pref_file_aux(player_ptr, path, PREF_TYPE_HISTPREF);
    world.character_xtra = old_character_xtra;
    return err;
}

/*!
 * @brief prfファイルのフォーマットに従った内容を出力する
 * @param fmt 出力内容
 */
void auto_dump_printf(FILE *auto_dump_stream, const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    const auto buf = vformat(fmt, vp);
    va_end(vp);

    // '\n'はSJISのマルチバイト文字のコードに含まれないため、ダメ文字を考慮する必要はない
    auto_dump_line_num += std::count(buf.begin(), buf.end(), '\n');

    fprintf(auto_dump_stream, "%s", buf.data());
}

/*!
 * @brief prfファイルをファイルオープンする /
 * Open file to append auto dump.
 * @param path ファイル名
 * @param mark 出力するヘッダマーク
 * @return ファイルポインタを取得できたらTRUEを返す
 */
bool open_auto_dump(FILE **fpp, const std::filesystem::path &path, std::string_view mark)
{
    const auto header_mark_str = format(auto_dump_header, mark.data());
    remove_auto_dump(path, mark);
    *fpp = angband_fopen(path, FileOpenMode::APPEND);
    if (!fpp) {
        const auto &path_str = path.string();
        msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), path_str.data());
        msg_erase();
        return false;
    }

    fprintf(*fpp, "%s\n", header_mark_str.data());
    auto_dump_line_num = 0;
    auto_dump_printf(*fpp, _("# *警告!!* 以降の行は自動生成されたものです。\n", "# *Warning!*  The lines below are an automatic dump.\n"));
    auto_dump_printf(
        *fpp, _("# *警告!!* 後で自動的に削除されるので編集しないでください。\n", "# Don't edit them; changes will be deleted and replaced automatically.\n"));
    return true;
}

/*!
 * @brief prfファイルをファイルクローズする /
 * Append foot part and close auto dump.
 * @param auto_dump_mark 出力するヘッダマーク
 */
void close_auto_dump(FILE **fpp, std::string_view mark)
{
    const auto footer_mark_str = format(auto_dump_footer, mark.data());
    auto_dump_printf(*fpp, _("# *警告!!* 以降の行は自動生成されたものです。\n", "# *Warning!*  The lines below are an automatic dump.\n"));
    auto_dump_printf(
        *fpp, _("# *警告!!* 後で自動的に削除されるので編集しないでください。\n", "# Don't edit them; changes will be deleted and replaced automatically.\n"));
    fprintf(*fpp, "%s (%d)\n", footer_mark_str.data(), auto_dump_line_num);
    angband_fclose(*fpp);
}

/*!
 * @brief 全ユーザプロファイルをロードする / Load some "user pref files"
 * @paaram player_ptr プレイヤーへの参照ポインタ
 * @note
 * Modified by Arcum Dagsson to support
 * separate macro files for different realms.
 */
void load_all_pref_files(PlayerType *player_ptr)
{
    process_pref_file(player_ptr, "user.prf");
    process_pref_file(player_ptr, format("user-%s.prf", ANGBAND_SYS));
    constexpr auto fmt = "%s.prf";
    process_pref_file(player_ptr, format(fmt, rp_ptr->title.data()));
    process_pref_file(player_ptr, format(fmt, cp_ptr->title.data()));
    process_pref_file(player_ptr, format(fmt, player_ptr->base_name));
    PlayerRealm pr(player_ptr);
    if (pr.realm1().is_available()) {
        process_pref_file(player_ptr, format(fmt, pr.realm1().get_name().data()));
    }

    if (pr.realm2().is_available()) {
        process_pref_file(player_ptr, format(fmt, pr.realm2().get_name().data()));
    }

    autopick_load_pref(player_ptr, false);
}

/*!
 * @brief 生い立ちメッセージをファイルからロードする。
 */
bool read_histpref(PlayerType *player_ptr)
{
    if (!input_check(_("生い立ち設定ファイルをロードしますか? ", "Load background history preference file? "))) {
        return false;
    }

    histpref_buf = "";
    std::stringstream ss;
    ss << _("histedit-", "histpref-") << player_ptr->base_name << ".prf";
    auto err = process_histpref_file(player_ptr, ss.str());
    if (0 > err) {
        err = process_histpref_file(player_ptr, _("histedit.prf", "histpref.prf"));
    }

    const auto finalizer = util::make_finalizer([]() { histpref_buf = std::nullopt; });
    if (err) {
        msg_print(_("生い立ち設定ファイルの読み込みに失敗しました。", "Failed to load background history preference."));
        msg_erase();
        return false;
    }

    if (!histpref_buf || histpref_buf->empty()) {
        msg_print(_("有効な生い立ち設定はこのファイルにありません。", "There does not exist valid background history preference."));
        msg_erase();
        return false;
    }

    for (auto i = 0; i < 4; i++) {
        player_ptr->history[i][0] = '\0';
    }

    histpref_buf = str_trim(*histpref_buf);
    constexpr auto max_line_len = sizeof(player_ptr->history[0]);
    const auto history_lines = shape_buffer(*histpref_buf, max_line_len);
    const auto max_lines = std::min<int>(4, history_lines.size());
    for (auto l = 0; l < max_lines; ++l) {
        angband_strcpy(player_ptr->history[l], history_lines[l], max_line_len);
    }

    for (auto i = 0; i < 4; i++) {
        /* loop */
        int j;
        for (j = 0; player_ptr->history[i][j]; j++) {
            ;
        }

        for (; j < 59; j++) {
            player_ptr->history[i][j] = ' ';
        }
        player_ptr->history[i][59] = '\0';
    }

    return true;
}
