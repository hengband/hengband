#include "cmd-io/cmd-gameoption.h"
#include "autopick/autopick.h"
#include "cmd-io/cmd-autopick.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "core/window-redrawer.h"
#include "floor/geometry.h"
#include "game-option/game-play-options.h"
#include "game-option/keymap-directory-getter.h"
#include "game-option/option-flags.h"
#include "game-option/option-types-table.h"
#include "game-option/special-options.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/sound-of-music.h"
#include "system/enums/game-option-page.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-symbol.h"
#include "world/world.h"

#define OPT_NUM 15

struct opts {
    char key;
    concptr name;
    int row;
};

static opts option_fields[OPT_NUM] = {
    { '1', _("    キー入力     オプション", "Input Options"), 3 },
    { '2', _("   マップ画面    オプション", "Map Screen Options"), 4 },
    { '3', _("  テキスト表示   オプション", "Text Display Options"), 5 },
    { '4', _("  ゲームプレイ   オプション", "Game-Play Options"), 6 },
    { '5', _("  行動中止関係   オプション", "Disturbance Options"), 7 },
    { '6', _("  簡易自動破壊   オプション", "Easy Auto-Destroyer Options"), 8 },
    { 'r', _("   プレイ記録    オプション", "Play record Options"), 9 },

    { 'p', _("自動拾いエディタ", "Auto-picker/destroyer editor"), 11 },
    { 'd', _(" 基本ウェイト量 ", "Base Delay Factor"), 12 },
    { 'h', _("低ヒットポイント", "Hitpoint Warning"), 13 },
    { 'm', _("  低魔力色閾値  ", "Mana Color Threshold"), 14 },
    { 'a', _("   自動セーブ    オプション", "Autosave Options"), 15 },
    { 'w', _("ウインドウフラグ", "Window Flags"), 16 },

    { 'b', _("      初期       オプション (参照のみ)", "Birth Options (Browse Only)"), 18 },
    { 'c', _("      詐欺       オプション", "Cheat Options"), 19 },
};

/*!
 * @brief セーブ頻度ターンの次の値を返す
 * @param current 現在のセーブ頻度ターン値
 * @return 次のセーブ頻度ターン値
 */
static int16_t toggle_frequency(int16_t current)
{
    switch (current) {
    case 0:
        return 50;
    case 50:
        return 100;
    case 100:
        return 250;
    case 250:
        return 500;
    case 500:
        return 1000;
    case 1000:
        return 2500;
    case 2500:
        return 5000;
    case 5000:
        return 10000;
    case 10000:
        return 25000;
    default:
        return 0;
    }
}

/*!
 * @brief 自動セーブオプションを変更するコマンドのメインルーチン
 * @param info 表示メッセージ
 */
static void do_cmd_options_autosave(PlayerType *player_ptr, concptr info)
{
    char ch;
    int i, k = 0, n = 2;
    term_clear();
    while (true) {
        prt(format(_("%s ( リターンで次へ, y/n でセット, F で頻度を入力, ESC で決定 ) ", "%s (RET to advance, y/n to set, 'F' for frequency, ESC to accept) "), info), 0, 0);
        for (i = 0; i < n; i++) {
            byte a = TERM_WHITE;
            if (i == k) {
                a = TERM_L_BLUE;
            }

            c_prt(a, format("%-48s: %s (%s)", autosave_info[i].o_desc, (*autosave_info[i].o_var ? _("はい  ", "yes") : _("いいえ", "no ")), autosave_info[i].o_text), i + 2, 0);
        }

        prt(format(_("自動セーブの頻度： %d ターン毎", "Timed autosave frequency: every %d turns"), autosave_freq), 5, 0);
        move_cursor(k + 2, 50);
        ch = inkey();
        switch (ch) {
        case ESCAPE: {
            return;
        }

        case '-':
        case '8': {
            k = (n + k - 1) % n;
            break;
        }

        case ' ':
        case '\n':
        case '\r':
        case '2': {
            k = (k + 1) % n;
            break;
        }

        case 'y':
        case 'Y':
        case '6': {

            (*autosave_info[k].o_var) = true;
            k = (k + 1) % n;
            break;
        }

        case 'n':
        case 'N':
        case '4': {
            (*autosave_info[k].o_var) = false;
            k = (k + 1) % n;
            break;
        }

        case 'f':
        case 'F': {
            autosave_freq = toggle_frequency(autosave_freq);
            prt(format(_("自動セーブの頻度： %d ターン毎", "Timed autosave frequency: every %d turns"), autosave_freq), 5, 0);
            break;
        }

        case '?': {
            FileDisplayer(player_ptr->name).display(true, _("joption.txt#Autosave", "option.txt#Autosave"), 0, 0);
            term_clear();
            break;
        }

        default: {
            bell();
            break;
        }
        }
    }
}

/*!
 * @brief 指定のサブウィンドウが指定のウィンドウフラグを持つか調べる
 * @param x ウィンドウ番号
 * @param y ウィンドウフラグ番号
 * @return 持つならTRUE、持たないかメインウィンドウならFALSE
 */
static bool has_window_flag(int x, int y)
{
    if (x == 0) {
        return false;
    }

    auto flag = i2enum<SubWindowRedrawingFlag>(y);
    return g_window_flags[x].has(flag);
}

/*!
 * @brief 指定のサブウィンドウに指定のウィンドウフラグをセットする
 * @param x ウィンドウ番号
 * @param y ウィンドウフラグ番号
 * @details
 * 未使用フラグはセットしない。
 */
static void set_window_flag(int x, int y)
{
    if (x == 0) {
        return;
    }

    auto flag = i2enum<SubWindowRedrawingFlag>(y);
    g_window_flags[x].set(flag);
}

/*!
 * @brief 指定のウィンドウフラグをサブウィンドウからクリアする
 * @param y ウィンドウフラグ番号
 */
static void clear_window_flag(int x, int y)
{
    g_window_flags[x].clear();
    if (x == 0) {
        return;
    }

    auto flag = i2enum<SubWindowRedrawingFlag>(y);
    for (auto &window_flag : g_window_flags) {
        window_flag.reset(flag);
    }
}

/*!
 * @brief ウィンドウオプションを変更するコマンドのメインルーチン /
 * Modify the "window" options
 */
static void do_cmd_options_win(PlayerType *player_ptr)
{
    int i, j, d;
    TERM_LEN y = 0;
    TERM_LEN x = 0;
    char ch;
    bool go = true;
    const auto old_flags = g_window_flags;
    term_clear();
    while (go) {
        prt(_("ウィンドウ・フラグ (<方向>で移動, 't'でON/OFF,'s'でON(他窓OFF), ESC)", "Window Flags (<dir>, <t>oggle, <s>et, ESC) "), 0, 0);
        for (j = 0; j < 8; j++) {
            byte a = TERM_WHITE;
            auto s = angband_term_name[j];
            if (j == x) {
                a = TERM_L_BLUE;
            }

            term_putstr(35 + j * 5 - strlen(s) / 2, 2 + j % 2, -1, a, s);
        }

        for (i = 0; i < 16; i++) {
            byte a = TERM_WHITE;
            concptr str = window_flag_desc[i];
            if (i == y) {
                a = TERM_L_BLUE;
            }

            if (!str) {
                str = _("(未使用)", "(Unused option)");
            }

            term_putstr(0, i + 5, -1, a, str);
            for (j = 0; j < 8; j++) {
                char c = '.';
                a = TERM_WHITE;
                if ((i == y) && (j == x)) {
                    a = TERM_L_BLUE;
                }

                if (g_window_flags[j].has(i2enum<SubWindowRedrawingFlag>(i))) {
                    c = 'X';
                }

                term_putch(35 + j * 5, i + 5, { a, c });
            }
        }

        bool has_flag = false;
        term_gotoxy(35 + x * 5, y + 5);
        ch = inkey();
        switch (ch) {
        case ESCAPE:
            go = false;
            break;
        case ' ':
        case 't':
        case 'T':
            has_flag = has_window_flag(x, y);
            g_window_flags[x].clear();
            if (x > 0 && !has_flag) {
                set_window_flag(x, y);
            }
            break;
        case 's':
        case 'S':
            if (x == 0) {
                break;
            }
            g_window_flags[x].clear();
            clear_window_flag(x, y);
            set_window_flag(x, y);
            break;
        case '?':
            FileDisplayer(player_ptr->name).display(true, _("joption.txt#Window", "option.txt#Window"), 0, 0);
            term_clear();
            break;
        default:
            d = get_keymap_dir(ch);
            x = (x + ddx[d] + 8) % 8;
            y = (y + ddy[d] + 16) % 16;
            if (!d) {
                bell();
            }
            break;
        }
    }

    for (auto term_index = 0U; term_index < angband_terms.size(); ++term_index) {
        term_type *old = game_term;
        if (!angband_terms[term_index]) {
            continue;
        }

        if (g_window_flags[term_index] == old_flags[term_index]) {
            continue;
        }

        term_activate(angband_terms[term_index]);
        term_clear();
        term_fresh();
        term_activate(old);
    }
}

/*!
 * @brief チートオプションを変更するコマンドのメインルーチン
 * Interact with some options for cheating
 * @param info 表示メッセージ
 */
static void do_cmd_options_cheat(const FloorType &floor, std::string_view player_name, std::string_view info)
{
    term_clear();
    auto k = 0U;
    const auto n = cheat_info.size();
    while (true) {
        prt(format(_("%s ( リターンで次へ, y/n でセット, ESC で決定 )", "%s (RET to advance, y/n to set, ESC to accept) "), info.data()), 0, 0);

#ifdef JP
        /* 詐欺オプションをうっかりいじってしまう人がいるようなので注意 */
        prt("                                 <<  注意  >>", 11, 0);
        prt("      詐欺オプションを一度でも設定すると、スコア記録が残らなくなります！", 12, 0);
        prt("      後に解除してもダメですので、勝利者を目指す方はここのオプションはい", 13, 0);
        prt("      じらないようにして下さい。", 14, 0);
#endif
        for (auto i = 0U; i < n; i++) {
            auto a = TERM_WHITE;
            if (i == k) {
                a = TERM_L_BLUE;
            }

            const auto yesno = *cheat_info[i].o_var ? _("はい  ", "yes") : _("いいえ", "no ");
            c_prt(enum2i(a), format("%-48s: %s (%s)", cheat_info[i].o_desc, yesno, cheat_info[i].o_text), i + 2, 0);
        }

        move_cursor(k + 2, 50);
        auto ch = inkey();
        auto dir = get_keymap_dir(ch);
        if ((dir == 2) || (dir == 4) || (dir == 6) || (dir == 8)) {
            ch = I2D(dir);
        }

        switch (ch) {
        case ESCAPE:
            return;
        case '-':
        case '8':
            k = (n + k - 1) % n;
            break;
        case ' ':
        case '\n':
        case '\r':
        case '2':
            k = (k + 1) % n;
            break;
        case 'y':
        case 'Y':
        case '6': {
            auto &world = AngbandWorld::get_instance();
            if (!world.noscore) {
                exe_write_diary(floor, DiaryKind::DESCRIPTION, 0,
                    _("詐欺オプションをONにして、スコアを残せなくなった。", "gave up sending score to use cheating options."));
            }

            world.noscore |= cheat_info[k].o_set * 256 + cheat_info[k].o_bit;
            *cheat_info[k].o_var = true;
            k = (k + 1) % n;
            break;
        }
        case 'n':
        case 'N':
        case '4':
            *cheat_info[k].o_var = false;
            k = (k + 1) % n;
            break;
        case '?':
            FileDisplayer(player_name).display(true, std::string(_("joption.txt#", "option.txt#")).append(cheat_info[k].o_text), 0, 0);
            term_clear();
            break;
        default:
            bell();
            break;
        }
    }
}

/*!
 * @brief ビットセットからゲームオプションを展開する / Extract option variables from bit sets
 */
void extract_option_vars(void)
{
    for (int i = 0; option_info[i].o_desc; i++) {
        int os = option_info[i].o_set;
        int ob = option_info[i].o_bit;
        if (option_info[i].o_var) {
            if (g_option_flags[os] & (1UL << ob)) {
                (*option_info[i].o_var) = true;
            } else {
                (*option_info[i].o_var) = false;
            }
        }
    }
}

/*!
 * @brief 標準オプションを変更するコマンドのメインルーチン /
 * Set or unset various options.
 * @details
 * <pre>
 * The user must use the "Ctrl-R" command to "adapt" to changes
 * in any options which control "visual" aspects of the game.
 * </pre>
 */
void do_cmd_options(PlayerType *player_ptr)
{
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

    char k;
    int d, skey;
    TERM_LEN i, y = 0;
    screen_save();
    const auto &world = AngbandWorld::get_instance();
    while (true) {
        int n = OPT_NUM;
        if (!world.noscore && !allow_debug_opts) {
            n--;
        }

        term_clear();
        prt(_("[ オプションの設定 ]", "Game options"), 1, 0);
        while (true) {
            for (i = 0; i < n; i++) {
                byte a = TERM_WHITE;
                if (i == y) {
                    a = TERM_L_BLUE;
                }
                term_putstr(5, option_fields[i].row, -1, a, format("(%c) %s", toupper(option_fields[i].key), option_fields[i].name));
            }

            prt(_("<方向>で移動, Enterで決定, ESCでキャンセル, ?でヘルプ: ", "Move to <dir>, Select to Enter, Cancel to ESC, ? to help: "), 21, 0);
            skey = inkey_special(true);
            if (!(skey & SKEY_MASK)) {
                k = (char)skey;
            } else {
                k = 0;
            }

            if (k == ESCAPE) {
                break;
            }

            if (angband_strchr("\n\r ", k)) {
                k = option_fields[y].key;
                break;
            }

            for (i = 0; i < n; i++) {
                if (tolower(k) == option_fields[i].key) {
                    break;
                }
            }

            if (i < n) {
                break;
            }

            if (k == '?') {
                break;
            }

            d = 0;
            if (skey == SKEY_UP) {
                d = 8;
            }
            if (skey == SKEY_DOWN) {
                d = 2;
            }
            y = (y + ddy[d] + n) % n;
            if (!d) {
                bell();
            }
        }

        if (k == ESCAPE) {
            break;
        }

        switch (k) {
        case '1': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_INPUT, _("キー入力オプション", "Input Options"));
            break;
        }
        case '2': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_MAPSCREEN, _("マップ画面オプション", "Map Screen Options"));
            break;
        }
        case '3': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_TEXT, _("テキスト表示オプション", "Text Display Options"));
            break;
        }
        case '4': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_GAMEPLAY, _("ゲームプレイ・オプション", "Game-Play Options"));
            break;
        }
        case '5': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_DISTURBANCE, _("行動中止関係のオプション", "Disturbance Options"));
            break;
        }
        case '6': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_AUTODESTROY, _("簡易自動破壊オプション", "Easy Auto-Destroyer Options"));
            break;
        }
        case 'R':
        case 'r': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_PLAYRECORD, _("プレイ記録オプション", "Play-record Options"));
            break;
        }
        case 'B':
        case 'b': {
            do_cmd_options_aux(player_ptr, OPT_PAGE_BIRTH,
                (!world.wizard || !allow_debug_opts) ? _("初期オプション(参照のみ)", "Birth Options(browse only)")
                                                     : _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
            break;
        }
        case 'C':
        case 'c': {
            if (!world.noscore && !allow_debug_opts) {
                bell();
                break;
            }

            do_cmd_options_cheat(*player_ptr->current_floor_ptr, player_ptr->name, _("詐欺師は決して勝利できない！", "Cheaters never win"));
            break;
        }
        case 'a':
        case 'A': {
            do_cmd_options_autosave(player_ptr, _("自動セーブ", "Autosave"));
            break;
        }
        case 'W':
        case 'w': {
            do_cmd_options_win(player_ptr);
            RedrawingFlagsUpdater::get_instance().fill_up_sub_flags();
            break;
        }
        case 'P':
        case 'p': {
            do_cmd_edit_autopick(player_ptr);
            break;
        }
        case 'D':
        case 'd': {
            clear_from(18);
            prt(format(_("現在ウェイト量(msec): %d", "Current Delay Factor(msec): %d"), delay_factor), 19, 0);
            constexpr auto prompt = _("コマンド: ウェイト量(msec)", "Command: Delay Factor(msec)");
            const auto new_delay_factor = input_integer(prompt, 0, 1000, delay_factor);
            if (new_delay_factor) {
                delay_factor = *new_delay_factor;
            }

            clear_from(18);
            break;
        }
        case 'H':
        case 'h': {
            clear_from(18);
            prt(_("コマンド: 低ヒットポイント警告", "Command: Hitpoint Warning"), 19, 0);
            while (true) {
                prt(format(_("現在の低ヒットポイント警告: %d0%%", "Current hitpoint warning: %d0%%"), hitpoint_warn), 22, 0);
                prt(_("低ヒットポイント警告 (0-9) ESCで決定: ", "Hitpoint Warning (0-9 or ESC to accept): "), 20, 0);
                k = inkey();
                if (k == ESCAPE) {
                    break;
                } else if (k == '?') {
                    FileDisplayer(player_ptr->name).display(true, _("joption.txt#Hitpoint", "option.txt#Hitpoint"), 0, 0);
                    term_clear();
                } else if (isdigit(k)) {
                    hitpoint_warn = D2I(k);
                } else {
                    bell();
                }
            }

            break;
        }
        case 'M':
        case 'm': {
            clear_from(18);
            prt(_("コマンド: 低魔力色閾値", "Command: Mana Color Threshold"), 19, 0);
            while (true) {
                prt(format(_("現在の低魔力色閾値: %d0%%", "Current mana color threshold: %d0%%"), mana_warn), 22, 0);
                prt(_("低魔力閾値 (0-9) ESCで決定: ", "Mana color Threshold (0-9 or ESC to accept): "), 20, 0);
                k = inkey();
                if (k == ESCAPE) {
                    break;
                } else if (k == '?') {
                    FileDisplayer(player_ptr->name).display(true, _("joption.txt#Manapoint", "option.txt#Manapoint"), 0, 0);
                    term_clear();
                } else if (isdigit(k)) {
                    mana_warn = D2I(k);
                } else {
                    bell();
                }
            }

            break;
        }
        case '?':
            FileDisplayer(player_ptr->name).display(true, _("joption.txt", "option.txt"), 0, 0);
            term_clear();
            break;
        default: {
            bell();
            break;
        }
        }

        msg_erase();
    }

    screen_load();
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EQUIPPY);
}

/*!
 * @brief 標準オプションを変更するコマンドのサブルーチン /
 * Interact with some options
 * @param page オプションページ番号
 * @param info 表示メッセージ
 */
void do_cmd_options_aux(PlayerType *player_ptr, game_option_page page, concptr info)
{
    char ch;
    int i, k = 0, n = 0, l;
    int opt[MAIN_TERM_MIN_ROWS]{};
    const auto &world = AngbandWorld::get_instance();
    const auto browse_only = (page == OPT_PAGE_BIRTH) && world.character_generated && (!world.wizard || !allow_debug_opts);
    for (i = 0; i < MAIN_TERM_MIN_ROWS; i++) {
        opt[i] = 0;
    }

    for (i = 0; option_info[i].o_desc; i++) {
        if (option_info[i].o_page == page) {
            opt[n++] = i;
        }
    }

    term_clear();
    while (true) {
        DIRECTION dir;
        constexpr auto command = _("%s (リターン:次, %sESC:終了, ?:ヘルプ) ", "%s (RET:next, %s, ?:help) ");
        prt(format(command, info, browse_only ? _("", "ESC:exit") : _("y/n:変更, ", "y/n:change, ESC:accept")), 0, 0);
        if (page == OPT_PAGE_AUTODESTROY) {
            constexpr auto mes = _("以下のオプションは、簡易自動破壊を使用するときのみ有効", "Following options will protect items from easy auto-destroyer.");
            c_prt(TERM_YELLOW, mes, 6, _(6, 3));
        }

        for (i = 0; i < n; i++) {
            byte a = TERM_WHITE;
            if (i == k) {
                a = TERM_L_BLUE;
            }

            const auto reply = *option_info[opt[i]].o_var ? _("はい  ", "yes") : _("いいえ", "no ");
            const auto label = format("%-48s: %s (%.19s)", option_info[opt[i]].o_desc, reply, option_info[opt[i]].o_text);
            if ((page == OPT_PAGE_AUTODESTROY) && i > 2) {
                c_prt(a, label, i + 5, 0);
            } else {
                c_prt(a, label, i + 2, 0);
            }
        }

        if ((page == OPT_PAGE_AUTODESTROY) && (k > 2)) {
            l = 3;
        } else {
            l = 0;
        }

        move_cursor(k + 2 + l, 50);
        ch = inkey();
        dir = get_keymap_dir(ch);
        if ((dir == 2) || (dir == 4) || (dir == 6) || (dir == 8)) {
            ch = I2D(dir);
        }

        switch (ch) {
        case ESCAPE: {
            return;
        }
        case '-':
        case '8': {
            k = (n + k - 1) % n;
            break;
        }
        case ' ':
        case '\n':
        case '\r':
        case '2': {
            k = (k + 1) % n;
            break;
        }
        case 'y':
        case 'Y':
        case '6': {
            if (browse_only) {
                break;
            }
            (*option_info[opt[k]].o_var) = true;
            k = (k + 1) % n;
            break;
        }
        case 'n':
        case 'N':
        case '4': {
            if (browse_only) {
                break;
            }
            (*option_info[opt[k]].o_var) = false;
            k = (k + 1) % n;
            break;
        }
        case 't':
        case 'T': {
            if (!browse_only) {
                (*option_info[opt[k]].o_var) = !(*option_info[opt[k]].o_var);
            }
            break;
        }
        case '?': {
            FileDisplayer(player_ptr->name).display(true, std::string(_("joption.txt#", "option.txt#")).append(option_info[opt[k]].o_text), 0, 0);
            term_clear();
            break;
        }
        default: {
            bell();
            break;
        }
        }
    }
}
