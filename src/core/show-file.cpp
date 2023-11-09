#include "core/show-file.h"
#include "core/asking-player.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "system/angband-exceptions.h"
#include "system/angband-version.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <sstream>
#include <string>
#include <string_view>

/*!
 * @brief ファイル内容の一行をコンソールに出力する
 * Display single line of on-line help file
 * @param str 出力する文字列
 * @param cy コンソールの行
 * @param shower 確認中
 * @details
 * <pre>
 * You can insert some special color tag to change text color.
 * Such as...
 * WHITETEXT [[[[y|SOME TEXT WHICH IS DISPLAYED IN YELLOW| WHITETEXT
 * A colored segment is between "[[[[y|" and the last "|".
 * You can use any single character in place of the "|".
 * </pre>
 * @todo 表示とそれ以外を分割する
 */
static void show_file_aux_line(std::string_view str, int cy, std::string_view shower)
{
    char lcstr[1024];
    concptr ptr;
    byte textcolor = TERM_WHITE;
    byte focuscolor = TERM_YELLOW;

    if (!shower.empty()) {
        strcpy(lcstr, str.data());
        str_tolower(lcstr);

        ptr = angband_strstr(lcstr, shower);
        textcolor = (ptr == nullptr) ? TERM_L_DARK : TERM_WHITE;
    }

    int cx = 0;
    term_gotoxy(cx, cy);

    static const char tag_str[] = "[[[[";
    byte color = textcolor;
    char in_tag = '\0';
    for (size_t i = 0; i < str.length();) {
        int len = strlen(&str[i]);
        int showercol = len + 1;
        int bracketcol = len + 1;
        int endcol = len;
        if (!shower.empty()) {
            ptr = angband_strstr(&lcstr[i], shower);
            if (ptr) {
                showercol = ptr - &lcstr[i];
            }
        }

        ptr = in_tag ? angband_strchr(&str[i], in_tag) : angband_strstr(&str[i], tag_str);
        if (ptr) {
            bracketcol = ptr - &str[i];
        }
        if (bracketcol < endcol) {
            endcol = bracketcol;
        }
        if (showercol < endcol) {
            endcol = showercol;
        }

        term_addstr(endcol, color, &str[i]);
        cx += endcol;
        i += endcol;

        if (!shower.empty() && (endcol == showercol)) {
            const auto showerlen = shower.length();
            term_addstr(showerlen, focuscolor, &str[i]);
            cx += showerlen;
            i += showerlen;
            continue;
        }

        if (endcol != bracketcol) {
            continue;
        }

        if (in_tag) {
            i++;
            in_tag = '\0';
            color = textcolor;
            continue;
        }

        i += sizeof(tag_str) - 1;
        color = color_char_to_attr(str[i]);
        if (color == 255 || str[i + 1] == '\0') {
            color = textcolor;
            term_addstr(-1, color, tag_str);
            cx += sizeof(tag_str) - 1;
            continue;
        }

        i++;
        in_tag = str[i];
        i++;
    }

    term_erase(cx, cy);
}

/*!
 * @brief ファイル内容をコンソールに出力する
 * Recursive file perusal.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param show_version TRUEならばコンソール上にゲームのバージョンを表示する
 * @param name ファイル名の文字列
 * @param what 内容キャプションの文字列
 * @param line 表示の現在行
 * @param mode オプション
 * @details
 * <pre>
 * Process various special text in the input file, including
 * the "menu" structures used by the "help file" system.
 * Return FALSE on 'q' to exit from a deep, otherwise TRUE.
 * </pre>
 * @todo 表示とそれ以外を分割する
 */
bool show_file(PlayerType *player_ptr, bool show_version, std::string_view name_with_tag, int initial_line, uint32_t mode, std::string_view what)
{
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, std::nullopt);

    const auto &[wid, hgt] = term_get_size();

    char hook[68][32]{};
    auto stripped_names = str_split(name_with_tag, '#');
    auto &name = stripped_names[0];
    auto tag = stripped_names.size() > 1 ? stripped_names[1] : "";
    std::filesystem::path path_reopen("");
    FILE *fff = nullptr;
    std::stringstream caption;
    if (!what.empty()) {
        caption << what;
        path_reopen = name;
        fff = angband_fopen(path_reopen, FileOpenMode::READ);
    }

    if (!fff) {
        caption.clear();
        caption << _("ヘルプ・ファイル'", "Help file '");
        caption << name << "'";
        path_reopen = path_build(ANGBAND_DIR_HELP, name);
        fff = angband_fopen(path_reopen, FileOpenMode::READ);
    }

    if (!fff) {
        caption.clear();
        caption << _("スポイラー・ファイル'", "Info file '");
        caption << name << "'";
        path_reopen = path_build(ANGBAND_DIR_INFO, name);
        fff = angband_fopen(path_reopen, FileOpenMode::READ);
    }

    if (!fff) {
        caption.clear();
        path_reopen = path_build(ANGBAND_DIR, name);
        caption << _("スポイラー・ファイル'", "Info file '");
        caption << name << "'";
        fff = angband_fopen(path_reopen, FileOpenMode::READ);
    }

    const auto open_error_mes = format(_("'%s'をオープンできません。", "Cannot open '%s'."), name.data());
    if (!fff) {
        THROW_EXCEPTION(std::runtime_error, open_error_mes);
    }

    const auto caption_str = caption.str();
    int skey;
    auto next = 0;
    auto back = 0;
    auto menu = false;
    char buf[1024]{};
    auto reverse = initial_line < 0;
    auto line = initial_line;
    while (true) {
        char *str = buf;
        if (angband_fgets(fff, buf, sizeof(buf))) {
            break;
        }
        if (!prefix(str, "***** ")) {
            next++;
            continue;
        }

        if ((str[6] == '[') && isalpha(str[7])) {
            int k = str[7] - 'A';
            menu = true;
            if ((str[8] == ']') && (str[9] == ' ')) {
                angband_strcpy(hook[k], str + 10, sizeof(hook[k]));
            }

            continue;
        }

        if (str[6] != '<') {
            continue;
        }

        size_t len = strlen(str);
        if (str[len - 1] == '>') {
            str[len - 1] = '\0';
            if (!tag.empty() && streq(str + 7, tag)) {
                line = next;
            }
        }
    }

    auto size = next;
    int rows = hgt - 4;
    if (line == -1) {
        line = ((size - 1) / rows) * rows;
    }

    term_clear();

    std::string find;
    std::string finder_str;
    std::string shower;
    std::string shower_str;
    while (true) {
        if (line >= size - rows) {
            line = size - rows;
        }
        if (line < 0) {
            line = 0;
        }

        if (next > line) {
            angband_fclose(fff);
            fff = angband_fopen(path_reopen, FileOpenMode::READ);
            if (!fff) {
                THROW_EXCEPTION(std::runtime_error, open_error_mes);
            }

            next = 0;
        }

        while (next < line) {
            if (angband_fgets(fff, buf, sizeof(buf))) {
                break;
            }
            if (prefix(buf, "***** ")) {
                continue;
            }
            next++;
        }

        int row_count;
        for (row_count = 0; row_count < rows;) {
            concptr str = buf;
            if (!row_count) {
                line = next;
            }
            if (angband_fgets(fff, buf, sizeof(buf))) {
                break;
            }
            if (prefix(buf, "***** ")) {
                continue;
            }
            next++;
            if (!find.empty() && !row_count) {
                char lc_buf[1024];
                strcpy(lc_buf, str);
                str_tolower(lc_buf);
                if (!str_find(lc_buf, find)) {
                    continue;
                }
            }

            find.clear();
            show_file_aux_line(str, row_count + 2, shower);
            row_count++;
        }

        while (row_count < rows) {
            term_erase(0, row_count + 2);
            row_count++;
        }

        if (!find.empty()) {
            bell();
            line = back;
            find.clear();
            continue;
        }

        if (show_version) {
            constexpr auto title = _("[%s, %s, %d/%d]", "[%s, %s, Line %d/%d]");
            prt(format(title, get_version().data(), caption_str.data(), line, size), 0, 0);
        } else {
            constexpr auto title = _("[%s, %d/%d]", "[%s, Line %d/%d]");
            prt(format(title, caption_str.data(), line, size), 0, 0);
        }

        if (size <= rows) {
            prt(_("[キー:(?)ヘルプ (ESC)終了]", "[Press ESC to exit.]"), hgt - 1, 0);
        } else {
#ifdef JP
            if (reverse) {
                prt("[キー:(RET/スペース)↑ (-)↓ (?)ヘルプ (ESC)終了]", hgt - 1, 0);
            } else {
                prt("[キー:(RET/スペース)↓ (-)↑ (?)ヘルプ (ESC)終了]", hgt - 1, 0);
            }
#else
            prt("[Press Return, Space, -, =, /, |, or ESC to exit.]", hgt - 1, 0);
#endif
        }

        skey = inkey_special(true);
        switch (skey) {
        case '?':
            if (name != _("jhelpinfo.txt", "helpinfo.txt")) {
                show_file(player_ptr, true, _("jhelpinfo.txt", "helpinfo.txt"), 0, mode);
            }

            break;
        case '=': {
            prt(_("強調: ", "Show: "), hgt - 1, 0);
            auto ask_result = askfor(80, shower_str);
            if (!ask_result) {
                break;
            }

            shower_str = *ask_result;
            if (shower_str.empty()) {
                shower.clear();
                break;
            }

            str_tolower(shower_str.data());
            shower = shower_str;
            break;
        }
        case '/':
        case KTRL('s'): {
            prt(_("検索: ", "Find: "), hgt - 1, 0);
            auto ask_result = askfor(80, finder_str);
            if (!ask_result) {
                break;
            }

            finder_str = *ask_result;
            if (finder_str.empty()) {
                shower.clear();
                break;
            }

            find = finder_str;
            back = line;
            line = line + 1;
            str_tolower(finder_str.data());
            shower = finder_str;
            break;
        }
        case '#': {
            prt(_("行: ", "Goto Line: "), hgt - 1, 0);
            constexpr auto initial_goto = "0";
            while (true) {
                const auto ask_result = askfor(10, initial_goto);
                if (!ask_result) {
                    break;
                }

                try {
                    line = std::stoi(*ask_result);
                    break;
                } catch (std::invalid_argument const &) {
                    prt(_("数値を入力して下さい。", "Please input numeric value."), hgt - 1, 0);
                } catch (std::out_of_range const &) {
                    prt(_("入力可能な数値の範囲を超えています。", "Input value overflows the maximum number."), hgt - 1, 0);
                }
            }

            break;
        }
        case SKEY_TOP:
            line = 0;
            break;
        case SKEY_BOTTOM:
            line = ((size - 1) / rows) * rows;
            break;
        case '%': {
            prt(_("ファイル・ネーム: ", "Goto File: "), hgt - 1, 0);
            const auto ask_result = askfor(80, _("jhelp.hlp", "help.hlp"));
            if (!ask_result) {
                break;
            }

            if (!show_file(player_ptr, true, *ask_result, 0, mode)) {
                skey = 'q';
            }

            break;
        }
        case '-':
            line = line + (reverse ? rows : -rows);
            if (line < 0) {
                line = 0;
            }

            break;
        case SKEY_PGUP:
            line = line - rows;
            if (line < 0) {
                line = 0;
            }

            break;
        case '\n':
        case '\r':
            line = line + (reverse ? -1 : 1);
            if (line < 0) {
                line = 0;
            }

            break;
        case '8':
        case SKEY_UP:
            line--;
            if (line < 0) {
                line = 0;
            }

            break;
        case '2':
        case SKEY_DOWN:
            line++;
            break;
        case ' ':
            line = line + (reverse ? -rows : rows);
            if (line < 0) {
                line = 0;
            }

            break;
        case SKEY_PGDOWN:
            line = line + rows;
            break;
        default:
            break;
        }

        if (menu) {
            auto key = -1;
            if (!(skey & SKEY_MASK) && isalpha(skey)) {
                key = skey - 'A';
            }

            if ((key > -1) && hook[key][0]) {
                /* Recurse on that file */
                if (!show_file(player_ptr, true, hook[key], 0, mode)) {
                    skey = 'q';
                }
            }
        }

        if (skey == '|') {
            const auto xtmp = input_string(_("ファイル名: ", "File name: "), 80);
            if (!xtmp) {
                continue;
            }

            angband_fclose(fff);
            fff = angband_fopen(path_reopen, FileOpenMode::READ);
            const auto &path_xtemp = path_build(ANGBAND_DIR_USER, xtmp.value());
            auto *ffp = angband_fopen(path_xtemp, FileOpenMode::WRITE);

            if (!(fff && ffp)) {
                msg_print(_("ファイルを開けません。", "Failed to open file."));
                skey = ESCAPE;
                break;
            }

            fprintf(ffp, "%s: %s\n", player_ptr->name, !what.empty() ? what.data() : caption_str.data());
            char buff[1024]{};
            while (!angband_fgets(fff, buff, sizeof(buff))) {
                angband_fputs(ffp, buff, 80);
            }
            angband_fclose(fff);
            angband_fclose(ffp);
            fff = angband_fopen(path_reopen, FileOpenMode::READ);
        }

        if ((skey == ESCAPE) || (skey == '<')) {
            break;
        }

        if (skey == KTRL('q')) {
            skey = 'q';
        }

        if (skey == 'q') {
            break;
        }
    }

    angband_fclose(fff);
    return skey != 'q';
}

/*
 * Convert string to lower case
 */
void str_tolower(char *str)
{
    for (; *str; str++) {
#ifdef JP
        if (iskanji(*str)) {
            str++;
            continue;
        }
#endif
        *str = (char)tolower(*str);
    }
}
