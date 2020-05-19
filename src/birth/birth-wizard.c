#include "system/angband.h"
#include "birth/birth-wizard.h"
#include "term/gameterm.h"
#include "player/player-sex.h"
#include "birth/birth-explanations-table.h"
#include "birth/auto-roller.h"
#include "main/sound-definitions-table.h"
#include "player/patron.h"
#include "world/world.h"
#include "birth/quick-start.h"
#include "birth/birth-util.h"
#include "cmd/cmd-help.h"
#include "birth/birth-select-race.h"
#include "birth/birth-select-class.h"
#include "birth/birth-select-personality.h"
#include "birth/birth-select-realm.h"
#include "birth/game-play-initializer.h"
#include "birth/birth-stat.h"
#include "birth/birth-body-spec.h"
#include "birth/history-generator.h"
#include "birth/history-editor.h"
#include "player/process-name.h"
#include "player/avatar.h"
#include "view/display-main-window.h" // 暫定。後で消す.
#include "view/display-birth.h" // 暫定。後で消す予定。
#include "view/display-player.h" // 暫定。後で消す.

/*!
 * オートローラーの内容を描画する間隔 /
 * How often the autoroller will update the display and pause
 * to check for user interuptions.
 * Bigger values will make the autoroller faster, but slower
 * system may have problems because the user can't stop the
 * autoroller for this number of rolls.
 */
#define AUTOROLLER_STEP 5431L

static void display_initial_birth_message(player_type *creature_ptr)
{
    Term_clear();
    put_str(_("名前  :", "Name  :"), 1, 26);
    put_str(_("性別        :", "Sex         :"), 3, 1);
    put_str(_("種族        :", "Race        :"), 4, 1);
    put_str(_("職業        :", "Class       :"), 5, 1);
    c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
    put_str(_("キャラクターを作成します。('S'やり直す, 'Q'終了, '?'ヘルプ)",
                "Make your charactor. ('S' Restart, 'Q' Quit, '?' Help)"), 8, 10);
    put_str(_("注意：《性別》の違いはゲーム上ほとんど影響を及ぼしません。",
                "Note: Your 'sex' does not have any significant gameplay effects."), 23, 5);
}

/*!
 * @brief プレイヤーの性別選択を行う / Player sex
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @buf 表示用バッファ
 * @return やり直すならFALSE、それ以外はTRUE
 */
static bool get_player_sex(player_type *creature_ptr, char *buf)
{
    const char p2 = ')';
    char cur[80];
    sprintf(cur, _("%c%c%s", "%c%c %s"), '*', p2, _("ランダム", "Random"));
    int k = -1;
    int cs = 0;
    int os = MAX_SEXES;
    while (TRUE) {
        if (cs != os) {
            put_str(cur, 12 + (os / 5), 2 + 15 * (os % 5));
            if (cs == MAX_SEXES)
                sprintf(cur, _("%c%c%s", "%c%c %s"), '*', p2, _("ランダム", "Random"));
            else {
                sp_ptr = &sex_info[cs];
                concptr str = sp_ptr->title;
                sprintf(cur, _("%c%c%s", "%c%c %s"), I2A(cs), p2, str);
            }

            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("性別を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a sex (%c-%c) ('=' for options): "), I2A(0), I2A(1));
        put_str(buf, 10, 10);
        char c = inkey();
        if (c == 'Q')
            birth_quit();
        if (c == 'S')
            return FALSE;
        if (c == ' ' || c == '\r' || c == '\n') {
            k = cs == MAX_SEXES ? randint0(MAX_SEXES) : cs;
            break;
        }

        if (c == '*') {
            k = randint0(MAX_SEXES);
            break;
        }

        if (c == '4') {
            if (cs > 0)
                cs--;
        }

        if (c == '6') {
            if (cs < MAX_SEXES)
                cs++;
        }

        k = (islower(c) ? A2I(c) : -1);
        if ((k >= 0) && (k < MAX_SEXES)) {
            cs = k;
            continue;
        } else
            k = -1;

        if (c == '?')
            do_cmd_help(creature_ptr);
        else if (c == '=') {
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
        } else if (c != '4' && c != '6')
            bell();
    }

    creature_ptr->psex = (byte)k;
    sp_ptr = &sex_info[creature_ptr->psex];
    c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 15);
    return TRUE;
}

/*!
 * @brief プレーヤーキャラ作成ウィザード
 * @details
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 * @return なし
 */
bool player_birth_wizard(player_type *creature_ptr, void (*process_autopick_file_command)(char *))
{
    display_initial_birth_message(creature_ptr);
    int n;
    const char p2 = ')';
    char buf[80];
    for (n = 0; n < MAX_SEXES; n++) {
        sp_ptr = &sex_info[n];
        sprintf(buf, _("%c%c%s", "%c%c %s"), I2A(n), p2, sp_ptr->title);
        put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
    }

    if(!get_player_sex(creature_ptr, buf))
        return FALSE;

    clear_from(10);
    creature_ptr->prace = 0;
    while (TRUE) {
        char temp[80 * 10];
        concptr t;
        if (!get_player_race(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(race_explanations[creature_ptr->prace], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < 10; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }
        if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        clear_from(10);
        c_put_str(TERM_WHITE, "              ", 4, 15);
    }

    clear_from(10);
    creature_ptr->pclass = 0;
    while (TRUE) {
        char temp[80 * 9];
        concptr t;
        if (!get_player_class(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(class_explanations[creature_ptr->pclass], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < 9; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        c_put_str(TERM_WHITE, "              ", 5, 15);
    }

    if (!get_player_realms(creature_ptr))
        return FALSE;

    creature_ptr->pseikaku = 0;
    while (TRUE) {
        char temp[80 * 8];
        concptr t;
        if (!get_player_personality(creature_ptr))
            return FALSE;

        clear_from(10);
        roff_to_buf(personality_explanations[creature_ptr->pseikaku], 74, temp, sizeof(temp));
        t = temp;
        for (int i = 0; i < A_MAX; i++) {
            if (t[0] == 0)
                break;
            else {
                prt(t, 12 + i, 3);
                t += strlen(t) + 1;
            }
        }

        if (get_check_strict(_("よろしいですか？", "Are you sure? "), CHECK_DEFAULT_Y))
            break;

        c_put_str(TERM_L_BLUE, creature_ptr->name, 1, 34);
        prt("", 1, 34 + strlen(creature_ptr->name));
    }

    clear_from(10);
    put_str("                                   ", 3, 40);
    put_str("                                   ", 4, 40);
    put_str("                                   ", 5, 40);

    screen_save();
    do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
    screen_load();
    if (autoroller || autochara)
        auto_round = 0L;

    if (autoroller)
        if (!get_stat_limits(creature_ptr))
            return FALSE;

    chara_limit_type chara_limit;
    initialize_chara_limit(&chara_limit);
    if (autochara)
        if (!get_chara_limits(creature_ptr, &chara_limit))
            return FALSE;

    clear_from(10);
    init_turn(creature_ptr);
    while (TRUE) {
        int col;
        col = 42;
        if (autoroller || autochara) {
            Term_clear();
            put_str(_("回数 :", "Round:"), 10, col + 13);
            put_str(_("(ESCで停止)", "(Hit ESC to stop)"), 12, col + 13);
        } else {
            get_stats(creature_ptr);
            get_ahw(creature_ptr);
            get_history(creature_ptr);
        }

        if (autoroller) {
            put_str(_("最小値", " Limit"), 2, col + 5);
            put_str(_("成功率", "  Freq"), 2, col + 13);
            put_str(_("現在値", "  Roll"), 2, col + 24);
            for (int i = 0; i < A_MAX; i++) {
                int j, m;
                put_str(stat_names[i], 3 + i, col);
                j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];
                m = adjust_stat(stat_limit[i], j);
                cnv_stat(m, buf);
                c_put_str(TERM_L_BLUE, buf, 3 + i, col + 5);
            }
        }

        while (autoroller || autochara) {
            bool accept = TRUE;
            get_stats(creature_ptr);
            auto_round++;
            if (auto_round >= 1000000000L) {
                auto_round = 1;
                if (autoroller) {
                    for (int i = 0; i < A_MAX; i++) {
                        stat_match[i] = 0;
                    }
                }
            }

            if (autoroller) {
                for (int i = 0; i < A_MAX; i++) {
                    if (creature_ptr->stat_max[i] >= stat_limit[i])
                        stat_match[i]++;
                    else
                        accept = FALSE;
                }
            }

            if (accept) {
                get_ahw(creature_ptr);
                get_history(creature_ptr);

                if (autochara) {
                    if ((creature_ptr->age < chara_limit.agemin) || (creature_ptr->age > chara_limit.agemax))
                        accept = FALSE;
                    if ((creature_ptr->ht < chara_limit.htmin) || (creature_ptr->ht > chara_limit.htmax))
                        accept = FALSE;
                    if ((creature_ptr->wt < chara_limit.wtmin) || (creature_ptr->wt > chara_limit.wtmax))
                        accept = FALSE;
                    if ((creature_ptr->sc < chara_limit.scmin) || (creature_ptr->sc > chara_limit.scmax))
                        accept = FALSE;
                }

                if (accept)
                    break;
            }

            bool flag = (!(auto_round % AUTOROLLER_STEP));
            if (flag) {
                birth_put_stats(creature_ptr);
                put_str(format("%10ld", auto_round), 10, col + 20);
                Term_fresh();
                inkey_scan = TRUE;
                if (inkey()) {
                    get_ahw(creature_ptr);
                    get_history(creature_ptr);
                    break;
                }
            }
        }

        if (autoroller || autochara)
            sound(SOUND_LEVEL);

        flush();

        BIT_FLAGS mode = 0;
        get_extra(creature_ptr, TRUE);
        get_money(creature_ptr);
        creature_ptr->chaos_patron = (s16b)randint0(MAX_PATRON);
        char c;
        bool prev = FALSE;
        while (TRUE) {
            creature_ptr->update |= (PU_BONUS | PU_HP);
            update_creature(creature_ptr);
            creature_ptr->chp = creature_ptr->mhp;
            creature_ptr->csp = creature_ptr->msp;
            display_player(creature_ptr, mode, map_name);
            Term_gotoxy(2, 23);
            char b1 = '[';
            Term_addch(TERM_WHITE, b1);
            Term_addstr(-1, TERM_WHITE, _("'r' 次の数値", "'r'eroll"));
            if (prev)
                Term_addstr(-1, TERM_WHITE, _(", 'p' 前の数値", "'p'previous"));

            if (mode)
                Term_addstr(-1, TERM_WHITE, _(", 'h' その他の情報", ", 'h' Misc."));
            else
                Term_addstr(-1, TERM_WHITE, _(", 'h' 生い立ちを表示", ", 'h'istory"));

            Term_addstr(-1, TERM_WHITE, _(", Enter この数値に決定", ", or Enter to accept"));
            char b2 = ']';
            Term_addch(TERM_WHITE, b2);
            c = inkey();
            if (c == 'Q')
                birth_quit();
            if (c == 'S')
                return FALSE;

            if (c == '\r' || c == '\n' || c == ESCAPE)
                break;
            if ((c == ' ') || (c == 'r'))
                break;

            if (prev && (c == 'p')) {
                load_prev_data(creature_ptr, TRUE);
                continue;
            }

            if ((c == 'H') || (c == 'h')) {
                mode = ((mode != 0) ? 0 : 1);
                continue;
            }

            if (c == '?') {
                show_help(creature_ptr, _("jbirth.txt#AutoRoller", "birth.txt#AutoRoller"));
                continue;
            } else if (c == '=') {
                screen_save();
                do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
                screen_load();
                continue;
            }

            bell();
        }

        if (c == '\r' || c == '\n' || c == ESCAPE)
            break;

        save_prev_data(creature_ptr, &previous_char);
        previous_char.quick_ok = FALSE;
        prev = TRUE;
    }

    clear_from(23);
    get_name(creature_ptr);
    process_player_name(creature_ptr, current_world_ptr->creating_savefile);
    edit_history(creature_ptr, process_autopick_file_command);
    get_max_stats(creature_ptr);
    get_virtues(creature_ptr);
    prt(_("[ 'Q' 中断, 'S' 初めから, Enter ゲーム開始 ]", "['Q'uit, 'S'tart over, or Enter to continue]"), 23, _(14, 10));

    char c = inkey();
    if (c == 'Q')
        birth_quit();

    if (c == 'S')
        return FALSE;

    init_dungeon_quests(creature_ptr);
    save_prev_data(creature_ptr, &previous_char);
    previous_char.quick_ok = TRUE;
    return TRUE;
}
