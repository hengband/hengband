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
#include "realm/realm-names-table.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/buffer-shaper.h"
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
    std::vector<char> file_read__buf(FILE_READ_BUFF_SIZE);
    std::string error_line;
    while (angband_fgets(fp, file_read__buf.data(), file_read__buf.size()) == 0) {
        line++;
        if (!file_read__buf[0]) {
            continue;
        }

#ifdef JP
        if (!iskanji(file_read__buf[0]))
#endif
            if (iswspace(file_read__buf[0])) {
                continue;
            }

        if (file_read__buf[0] == '#') {
            continue;
        }
        error_line = file_read__buf.data();

        /* Process "?:<expr>" */
        if ((file_read__buf[0] == '?') && (file_read__buf[1] == ':')) {
            char f;
            char *s;
            s = file_read__buf.data() + 2;
            concptr v = process_pref_file_expr(player_ptr, &s, &f);
            bypass = streq(v, "0");
            continue;
        }

        if (bypass) {
            continue;
        }

        /* Process "%:<file>" */
        if (file_read__buf[0] == '%') {
            static int depth_count = 0;
            if (depth_count > 20) {
                continue;
            }

            depth_count++;
            switch (preftype) {
            case PREF_TYPE_AUTOPICK:
                (void)process_autopick_file(player_ptr, file_read__buf.data() + 2);
                break;
            case PREF_TYPE_HISTPREF:
                (void)process_histpref_file(player_ptr, file_read__buf.data() + 2);
                break;
            default:
                (void)process_pref_file(player_ptr, file_read__buf.data() + 2);
                break;
            }

            depth_count--;
            continue;
        }

        err = interpret_pref_file(player_ptr, file_read__buf.data());
        if (err != 0) {
            if (preftype != PREF_TYPE_AUTOPICK) {
                break;
            }

            process_autopick_file_command(file_read__buf.data());
            err = 0;
        }
    }

    if (err != 0) {
        /* Print error message */
        /* ToDo: Add better error messages */
        const auto &name_str = name.string();
        msg_format(_("ファイル'%s'の%d行でエラー番号%dのエラー。", "Error %d in line %d of file '%s'."), _(name_str.data(), err), line, _(err, name_str.data()));
        msg_format(_("('%s'を解析中)", "Parsing '%s'"), error_line.data());
        msg_print(nullptr);
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
        const auto &path = path_build(ANGBAND_DIR_PREF, name);
        err1 = process_pref_file_aux(player_ptr, path, PREF_TYPE_NORMAL);
        if (err1 > 0) {
            return err1;
        }
    }

    const auto &path = path_build(ANGBAND_DIR_USER, name);
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
    const auto &path = path_build(ANGBAND_DIR_USER, name);
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
    bool old_character_xtra = w_ptr->character_xtra;
    const auto &path = path_build(ANGBAND_DIR_USER, name);
    w_ptr->character_xtra = true;
    errr err = process_pref_file_aux(player_ptr, path, PREF_TYPE_HISTPREF);
    w_ptr->character_xtra = old_character_xtra;
    return err;
}

/*!
 * @brief prfファイルのフォーマットに従った内容を出力する /
 * Dump a formatted line, using "vstrnfmt()".
 * @param fmt 出力内容
 */
void auto_dump_printf(FILE *auto_dump_stream, const char *fmt, ...)
{
    va_list vp;
    char buf[1024];
    va_start(vp, fmt);
    (void)vstrnfmt(buf, sizeof(buf), fmt, vp);
    va_end(vp);
    for (auto p = buf; *p; p++) {
        if (*p == '\n') {
            auto_dump_line_num++;
        }
    }

    fprintf(auto_dump_stream, "%s", buf);
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
    char header_mark_str[80];
    strnfmt(header_mark_str, sizeof(header_mark_str), auto_dump_header, mark.data());
    remove_auto_dump(path, mark);
    *fpp = angband_fopen(path, FileOpenMode::APPEND);
    if (!fpp) {
        const auto &path_str = path.string();
        msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), path_str.data());
        msg_print(nullptr);
        return false;
    }

    fprintf(*fpp, "%s\n", header_mark_str);
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
    char footer_mark_str[80];
    strnfmt(footer_mark_str, sizeof(footer_mark_str), auto_dump_footer, mark.data());
    auto_dump_printf(*fpp, _("# *警告!!* 以降の行は自動生成されたものです。\n", "# *Warning!*  The lines below are an automatic dump.\n"));
    auto_dump_printf(
        *fpp, _("# *警告!!* 後で自動的に削除されるので編集しないでください。\n", "# Don't edit them; changes will be deleted and replaced automatically.\n"));
    fprintf(*fpp, "%s (%d)\n", footer_mark_str, auto_dump_line_num);
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
    process_pref_file(player_ptr, std::string("user-").append(ANGBAND_SYS).append(".prf"));
    process_pref_file(player_ptr, std::string(rp_ptr->title).append(".prf"));
    process_pref_file(player_ptr, std::string(cp_ptr->title).append(".prf"));
    process_pref_file(player_ptr, std::string(player_ptr->base_name).append(".prf"));
    if (player_ptr->realm1 != REALM_NONE) {
        process_pref_file(player_ptr, std::string(realm_names[player_ptr->realm1]).append(".prf"));
    }

    if (player_ptr->realm2 != REALM_NONE) {
        process_pref_file(player_ptr, std::string(realm_names[player_ptr->realm2]).append(".prf"));
    }

    autopick_load_pref(player_ptr, false);
}

/*!
 * @brief 生い立ちメッセージをファイルからロードする。
 */
bool read_histpref(PlayerType *player_ptr)
{
    errr err;
    int i, j, n;
    char *s;
    char histbuf[HISTPREF_LIMIT];

    if (!get_check(_("生い立ち設定ファイルをロードしますか? ", "Load background history preference file? "))) {
        return false;
    }

    histbuf[0] = '\0';
    histpref_buf = histbuf;

    err = process_histpref_file(player_ptr, std::string(_("histedit-", "histpref-")).append(player_ptr->base_name).append(".prf").data());

    if (0 > err) {
        err = process_histpref_file(player_ptr, _("histedit.prf", "histpref.prf"));
    }

    if (err) {
        msg_print(_("生い立ち設定ファイルの読み込みに失敗しました。", "Failed to load background history preference."));
        msg_print(nullptr);
        histpref_buf = nullptr;
        return false;
    } else if (!histpref_buf[0]) {
        msg_print(_("有効な生い立ち設定はこのファイルにありません。", "There does not exist valid background history preference."));
        msg_print(nullptr);
        histpref_buf = nullptr;
        return false;
    }

    for (i = 0; i < 4; i++) {
        player_ptr->history[i][0] = '\0';
    }

    /* loop */
    for (s = histpref_buf; *s == ' '; s++) {
        ;
    }

    n = strlen(s);
    while ((n > 0) && (s[n - 1] == ' ')) {
        s[--n] = '\0';
    }

    constexpr auto max_line_len = sizeof(player_ptr->history[0]);
    const auto history_lines = shape_buffer(s, max_line_len);
    const auto max_lines = std::min<int>(4, history_lines.size());
    for (auto l = 0; l < max_lines; ++l) {
        angband_strcpy(player_ptr->history[l], history_lines[l].data(), max_line_len);
    }

    for (i = 0; i < 4; i++) {
        /* loop */
        for (j = 0; player_ptr->history[i][j]; j++) {
            ;
        }

        for (; j < 59; j++) {
            player_ptr->history[i][j] = ' ';
        }
        player_ptr->history[i][59] = '\0';
    }

    histpref_buf = nullptr;
    return true;
}
