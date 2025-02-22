/*!
 * @brief 記念撮影のセーブとロード
 * @date 2020/04/22
 * @Author Hourier
 */

#include "cmd-io/cmd-process-screen.h"
#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "game-option/special-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "system/angband-exceptions.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include <optional>
#include <string>
#include <string_view>

// Encode the screen colors
static char hack[17] = "dwsorgbuDWvyRGBU";

static concptr tags[4] = {
    "HEADER_START:",
    "HEADER_END:",
    "FOOTER_START:",
    "FOOTER_END:",
};
static concptr html_head[3] = {
    "<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n",
    "<pre>",
    0,
};
static concptr html_foot[3] = {
    "</pre>\n",
    "</body>\n</html>\n",
    0,
};

/*!
 * @brief 一時ファイルを読み込み、ファイルに書き出す
 * @param fff ファイルへの参照ポインタ
 * @param tempfff 一時ファイルへの参照ポインタ
 * @param num_tag タグ番号
 * @todo io/ 以下に移したいところだが、このファイルの行数も大したことがないので一旦保留
 */
static void read_temporary_file(FILE *fff, FILE *tmpfff, int num_tag)
{
    bool is_first_line = true;
    int next_tag = num_tag + 1;
    while (true) {
        const auto buf = angband_fgets(tmpfff);
        if (!buf) {
            break;
        }
        if (is_first_line) {
            if (strncmp(buf->data(), tags[num_tag], strlen(tags[num_tag])) == 0) {
                is_first_line = false;
            }

            continue;
        }

        if (strncmp(buf->data(), tags[next_tag], strlen(tags[next_tag])) == 0) {
            break;
        }

        fprintf(fff, "%s\n", buf->data());
    }
}

/*!
 * @brief 記念撮影を1行ダンプする
 * @param wid 幅
 * @param y 現在の行位置
 * @param fff 記念撮影ファイルへの参照ポインタ
 */
static void screen_dump_one_line(int wid, int y, FILE *fff)
{
    uint8_t old_a = 0;
    DisplaySymbol ds(0, ' ');
    for (TERM_LEN x = 0; x < wid - 1; x++) {
        concptr cc = nullptr;
        ds = term_what(x, y, ds);
        switch (ds.character) {
        case '&':
            cc = "&amp;";
            break;
        case '<':
            cc = "&lt;";
            break;
        case '>':
            cc = "&gt;";
            break;
#ifdef WINDOWS
        case 0x1f:
            ds.character = '.';
            break;
        case 0x7f:
            ds.character = (ds.color == 0x09) ? '%' : '#';
            break;
#endif
        }

        ds.color = ds.color & 0x0F;
        if (((y == 0) && (x == 0)) || (ds.color != old_a)) {
            int rv = angband_color_table[ds.color][1];
            int gv = angband_color_table[ds.color][2];
            int bv = angband_color_table[ds.color][3];
            fprintf(fff, "%s<font color=\"#%02x%02x%02x\">",
                ((y == 0 && x == 0) ? "" : "</font>"), rv, gv, bv);
            old_a = ds.color;
        }

        if (cc) {
            fprintf(fff, "%s", cc);
        } else {
            fprintf(fff, "%c", ds.character);
        }
    }
}

/*!
 * @brief 記念撮影を行方向にスイープする
 * @param wid 幅
 * @param hgt 高さ
 * @param fff 記念撮影ファイルへの参照ポインタ
 */
static void screen_dump_lines(int wid, int hgt, FILE *fff)
{
    for (TERM_LEN y = 0; y < hgt; y++) {
        if (y != 0) {
            fprintf(fff, "\n");
        }

        screen_dump_one_line(wid, y, fff);
    }
}

/*!
 * @brief ファイルへ書き込めない場合にエラーを表示する
 * @param fff ダンプファイルへの参照ポインタ
 * @param path 保存先HTMLファイルのパス
 * @param need_message メッセージ表示の必要性
 * @return メッセージを表示して後続処理を実行するか否か
 */
static bool check_screen_html_can_open(FILE *fff, const std::filesystem::path &path, bool need_message)
{
    if (fff) {
        return true;
    }

    if (!need_message) {
        return false;
    }

    const auto &path_str = path.string();
    const auto mes = format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), path_str.data());
    THROW_EXCEPTION(std::runtime_error, mes);
}

/*!
 * @brief HTMLヘッダを書き込む
 * @param tmpfff 一時ファイルへの参照ポインタ
 * @param fff 記念撮影ファイルへの参照ポインタ
 */
static void write_html_header(FILE *tmpfff, FILE *fff)
{
    if (tmpfff) {
        read_temporary_file(fff, tmpfff, 0);
        return;
    }

    for (int i = 0; html_head[i]; i++) {
        fputs(html_head[i], fff);
    }
}

/*!
 * @brief HTMLフッタを書き込む
 * @param tmpfff 一時ファイルへの参照ポインタ
 * @param fff 記念撮影ファイルへの参照ポインタ
 */
static void write_html_footer(FILE *tmpfff, FILE *fff)
{
    fprintf(fff, "</font>");
    if (!tmpfff) {
        for (int i = 0; html_foot[i]; i++) {
            fputs(html_foot[i], fff);
        }
    } else {
        rewind(tmpfff);
        read_temporary_file(fff, tmpfff, 2);
        angband_fclose(tmpfff);
    }

    fprintf(fff, "\n");
}

void exe_cmd_save_screen_html(const std::filesystem::path &path, bool need_message)
{
    const auto &[wid, hgt] = term_get_size();
    auto *fff = angband_fopen(path, FileOpenMode::WRITE);
    if (!check_screen_html_can_open(fff, path, need_message)) {
        return;
    }

    if (need_message) {
        screen_save();
    }

    const auto path_prf = path_build(ANGBAND_DIR_USER, "htmldump.prf");
    auto *tmpfff = angband_fopen(path_prf, FileOpenMode::READ);
    write_html_header(tmpfff, fff);
    screen_dump_lines(wid, hgt, fff);
    write_html_footer(tmpfff, fff);
    angband_fclose(fff);
    if (!need_message) {
        return;
    }

    msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
    msg_print(nullptr);
    screen_load();
}

/*!
 * @brief HTML方式で記念撮影する / Save a screen dump to a file
 * @param なし
 */
static void exe_cmd_save_screen_html_with_naming()
{
    const auto filename = input_string(_("ファイル名: ", "File name: "), 80, "screen.html");
    if (!filename) {
        return;
    }

    const auto path = path_build(ANGBAND_DIR_USER, *filename);
    msg_print(nullptr);
    exe_cmd_save_screen_html(path, true);
}

/*!
 * @brief 記念撮影の方式を問い合わせる
 * @param html_dump HTMLダンプするか否か
 * @return ダンプするならTRUE、キャンセルならFALSE
 */
static bool ask_html_dump(bool *html_dump)
{
    while (true) {
        char c = inkey();
        if (c == 'Y' || c == 'y') {
            *html_dump = false;
            return true;
        }

        if (c == 'H' || c == 'h') {
            *html_dump = true;
            return true;
        }

        prt("", 0, 0);
        return false;
    }

    // コンパイル警告対応.
    return false;
}

/*!
 * @brief ファイルへ書き込めない場合にエラーを表示する
 * @param fff ダンプファイルへの参照ポインタ
 * @return ファイルへ書き込めるならTRUE、書き込めないならFALSE
 */
static bool check_screen_text_can_open(FILE *fff, const std::string_view filename)
{
    if (fff) {
        return true;
    }

    msg_format(_("ファイル %s を開けませんでした。", "Failed to open file %s."), filename.data());
    msg_print(nullptr);
    return false;
}

/*!
 * @brief テキスト方式で記念撮影する
 * @param wid 幅
 * @param hgt 高さ
 * @return 記念撮影に成功したらTRUE、ファイルが開けなかったらFALSE
 * @todo どこかバグっていて、(恐らく初期化されていない)変な文字列まで出力される
 */
static bool do_cmd_save_screen_text(int wid, int hgt)
{
    DisplaySymbol ds(0, ' ');
    const auto path = path_build(ANGBAND_DIR_USER, "dump.txt");
    auto *fff = angband_fopen(path, FileOpenMode::WRITE);
    if (!check_screen_text_can_open(fff, path.string())) {
        return false;
    }

    screen_save();
    for (TERM_LEN y = 0; y < hgt; y++) {
        TERM_LEN x;
        char buf[1024]{};
        for (x = 0; x < wid - 1; x++) {
            ds = term_what(x, y, ds);
            buf[x] = ds.character;
        }

        buf[x] = '\0';
        fprintf(fff, "%s\n", buf);
    }

    fprintf(fff, "\n");
    for (TERM_LEN y = 0; y < hgt; y++) {
        TERM_LEN x;
        char buf[1024]{};
        for (x = 0; x < wid - 1; x++) {
            ds = term_what(x, y, ds);
            buf[x] = hack[ds.color & 0x0F];
        }

        buf[x] = '\0';
        fprintf(fff, "%s\n", buf);
    }

    fprintf(fff, "\n");
    angband_fclose(fff);
    msg_print(_("画面(記念撮影)をファイルに書き出しました。", "Screen dump saved."));
    msg_print(nullptr);
    screen_load();
    return true;
}

/*!
 * @brief 記念撮影のためにグラフィック使用をOFFにする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 記念撮影直前のグラフィックオプション
 */
static bool update_use_graphics(PlayerType *player_ptr)
{
    if (!use_graphics) {
        return true;
    }

    use_graphics = false;
    reset_visuals(player_ptr);
    static constexpr auto flags = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::EQUIPPY,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    handle_stuff(player_ptr);
    return false;
}

/*
 * Save a screen dump to a file
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_save_screen(PlayerType *player_ptr)
{
    prt(_("記念撮影しますか？ [(y)es/(h)tml/(n)o] ", "Save screen dump? [(y)es/(h)tml/(n)o] "), 0, 0);
    bool html_dump;
    if (!ask_html_dump(&html_dump)) {
        return;
    }

    const auto &[wid, hgt] = term_get_size();
    const auto old_use_graphics = update_use_graphics(player_ptr);

    if (html_dump) {
        exe_cmd_save_screen_html_with_naming();
        do_cmd_redraw(player_ptr);
    } else if (!do_cmd_save_screen_text(wid, hgt)) {
        return;
    }

    if (old_use_graphics) {
        return;
    }

    use_graphics = true;
    reset_visuals(player_ptr);
    static constexpr auto flags = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::MAP,
        MainWindowRedrawingFlag::EQUIPPY,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    handle_stuff(player_ptr);
}

/*!
 * @brief 白文字だけ画面に描画する
 * @param buf 描画用バッファ
 * @param fff 記念撮影ファイルへの参照ポインタ
 * @param wid 幅
 * @param hgt 高さ
 * @todo 目的は不明瞭
 * @return ファイルが読み込めなくなったらFALSEで抜ける
 */
static bool draw_white_characters(FILE *fff, int wid, int hgt)
{
    bool okay = true;
    for (TERM_LEN y = 0; okay; y++) {
        char buf[1024]{};
        if (!fgets(buf, sizeof(buf), fff)) {
            okay = false;
        }

        if (buf[0] == '\n' || buf[0] == '\0') {
            break;
        }
        if (y >= hgt) {
            continue;
        }

        for (TERM_LEN x = 0; x < wid - 1; x++) {
            if (buf[x] == '\n' || buf[x] == '\0') {
                break;
            }

            term_draw(x, y, TERM_WHITE, buf[x]);
        }
    }

    return okay;
}

/*!
 * @brief 白以外の文字を画面に描画する
 * @param fff 記念撮影ファイルへの参照ポインタ
 * @param wid 幅
 * @param hgt 高さ
 * @param 白文字が途中で読み込めなくなっていたらTRUE
 * @todo 目的は不明瞭
 */
static void draw_colored_characters(FILE *fff, int wid, int hgt, bool okay)
{
    DisplaySymbol ds(TERM_DARK, ' ');
    for (TERM_LEN y = 0; okay; y++) {
        char buf[1024]{};
        if (!fgets(buf, sizeof(buf), fff)) {
            okay = false;
        }

        if (buf[0] == '\n' || buf[0] == '\0') {
            break;
        }
        if (y >= hgt) {
            continue;
        }

        for (TERM_LEN x = 0; x < wid - 1; x++) {
            if (buf[x] == '\n' || buf[x] == '\0') {
                break;
            }

            ds = term_what(x, y, ds);
            for (uint8_t i = 0; i < 16; i++) {
                if (hack[i] == buf[x]) {
                    ds.color = i;
                }
            }

            term_draw(x, y, ds.color, ds.character);
        }
    }
}

/*
 * @brief Load a screen dump from a file
 * @param なし
 */
void do_cmd_load_screen(void)
{
    const auto &[wid, hgt] = term_get_size();
    const auto path = path_build(ANGBAND_DIR_USER, "dump.txt");
    auto *fff = angband_fopen(path, FileOpenMode::READ);
    if (!fff) {
        const auto filename = path.string();
        msg_format(_("%s を開くことができませんでした。", "Failed to open %s."), filename.data());
        msg_print(nullptr);
        return;
    }

    screen_save();
    term_clear();
    bool okay = draw_white_characters(fff, wid, hgt);
    draw_colored_characters(fff, wid, hgt, okay);

    angband_fclose(fff);
    prt(_("ファイルに書き出された画面(記念撮影)をロードしました。", "Screen dump loaded."), 0, 0);
    flush();
    inkey();
    screen_load();
}
