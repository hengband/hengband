/*!
 * @file birth.c
 * @brief プレイヤーの作成を行う / Create a player character
 * @date 2013/12/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "system/angband.h"
#include "term/gameterm.h"
#include "io/read-pref-file.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "market/building.h"
#include "util/util.h"
#include "player/avatar.h"
#include "birth/birth.h"
#include "birth/birth-explanations-table.h"
#include "cmd/cmd-help.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "birth/history.h"
#include "io/write-diary.h"
#include "market/store.h"
#include "monster/monster.h"
#include "monster/monster-race.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "realm/realm.h"
#include "io/save.h"
#include "spell/spells-util.h"
#include "view/display-main-window.h" // 暫定。後で消す.
#include "view/display-player.h" // 暫定。後で消す.
#include "floor/wild.h"
#include "world/world.h"
#include "birth/birth-util.h"
#include "birth/birth-select-realm.h"
#include "birth/quick-start.h"
#include "birth/birth-stat.h"
#include "birth/history-generator.h"
#include "birth/birth-body-spec.h"
#include "view/display-birth.h" // 暫定。後で消す予定。
#include "birth/inventory-initializer.h"
#include "birth/game-play-initializer.h"
#include "birth/history-editor.h"
#include "io/read-pref-file.h"
#include "birth/birth-select-race.h"
#include "birth/birth-select-class.h"
#include "birth/birth-select-personality.h"

/*!
 * オートローラーの内容を描画する間隔 /
 * How often the autoroller will update the display and pause
 * to check for user interuptions.
 * Bigger values will make the autoroller faster, but slower
 * system may have problems because the user can't stop the
 * autoroller for this number of rolls.
 */
#define AUTOROLLER_STEP 5431L

/*!
  * ランダムクエストのモンスターを確定するために試行する回数 / Maximum number of tries for selection of a proper quest monster
  */
#define MAX_TRIES 100

/*! オートローラの能力値的要求水準 / Autoroll limit */
static s16b stat_limit[6];

/*! オートローラの年齢、身長、体重、社会的地位の要求水準 */
static struct {
    s16b agemin, agemax;
    s16b htmin, htmax;
    s16b wtmin, wtmax;
    s16b scmin, scmax;
} chara_limit;

/*! オートローラ中、各能力値が水準を超えた回数 / Autoroll matches */
s32b stat_match[6];

/*! オートローラの試行回数 / Autoroll round */
s32b auto_round;

/*!
 * @brief オートローラで得たい能力値の基準を決める。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static bool get_stat_limits(player_type* creature_ptr)
{
    clear_from(10);
    put_str(_("最低限得たい能力値を設定して下さい。", "Set minimum stats."), 10, 10);
    put_str(_("2/8で項目選択、4/6で値の増減、Enterで次へ", "2/8 for Select, 4/6 for Change value, Enter for Goto next"), 11, 10);
    put_str(_("         基本値  種族 職業 性格     合計値  最大値", "           Base   Rac  Cla  Per      Total  Maximum"), 13, 10);

    int cval[6];
    char buf[80];
    char cur[80];
    char inp[80];
    for (int i = 0; i < A_MAX; i++) {
        stat_match[i] = 0;
        cval[i] = 3;
        int j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];
        int m = adjust_stat(17, j);
        if (m > 18)
            sprintf(cur, "18/%02d", (m - 18));
        else
            sprintf(cur, "%2d", m);

        m = adjust_stat(cval[i], j);
        if (m > 18)
            sprintf(inp, "18/%02d", (m - 18));
        else
            sprintf(inp, "%2d", m);

        sprintf(buf, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s  %6s",
            stat_names[i], cval[i], rp_ptr->r_adj[i], cp_ptr->c_adj[i],
            ap_ptr->a_adj[i], inp, cur);
        put_str(buf, 14 + i, 10);
    }

    int cs = 0;
    int os = 6;
    while (TRUE) {
        if (cs != os) {
            if (os == 6) {
                c_put_str(TERM_WHITE, _("決定する", "Accept"), 21, 35);
            } else if (os < A_MAX) {
                c_put_str(TERM_WHITE, cur, 14 + os, 10);
            }
            if (cs == 6) {
                c_put_str(TERM_YELLOW, _("決定する", "Accept"), 21, 35);
            } else {
                int j = rp_ptr->r_adj[cs] + cp_ptr->c_adj[cs] + ap_ptr->a_adj[cs];
                int m = adjust_stat(cval[cs], j);
                if (m > 18)
                    sprintf(inp, "18/%02d", (m - 18));
                else
                    sprintf(inp, "%2d", m);

                sprintf(cur, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s",
                    stat_names[cs], cval[cs], rp_ptr->r_adj[cs],
                    cp_ptr->c_adj[cs], ap_ptr->a_adj[cs], inp);
                c_put_str(TERM_YELLOW, cur, 14 + cs, 10);
            }

            os = cs;
        }

        char c = inkey();
        switch (c) {
        case 'Q':
            birth_quit();
            break;
        case 'S':
            return FALSE;
        case ESCAPE:
            break;
        case ' ':
        case '\r':
        case '\n':
            if (cs == 6)
                break;
            cs++;
            c = '2';
            break;
        case '8':
        case 'k':
            if (cs > 0)
                cs--;
            break;
        case '2':
        case 'j':
            if (cs < A_MAX)
                cs++;
            break;
        case '4':
        case 'h':
            if (cs != 6) {
                if (cval[cs] == 3) {
                    cval[cs] = 17;
                    os = 7;
                } else if (cval[cs] > 3) {
                    cval[cs]--;
                    os = 7;
                } else
                    return FALSE;
            }

            break;
        case '6':
        case 'l':
            if (cs != 6) {
                if (cval[cs] == 17) {
                    cval[cs] = 3;
                    os = 7;
                } else if (cval[cs] < 17) {
                    cval[cs]++;
                    os = 7;
                } else
                    return FALSE;
            }

            break;
        case 'm':
            if (cs != 6) {
                cval[cs] = 17;
                os = 7;
            }

            break;
        case 'n':
            if (cs != 6) {
                cval[cs] = 3;
                os = 7;
            }

            break;
        case '?':
#ifdef JP
            show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
            show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
            break;
        case '=':
            screen_save();
#ifdef JP
            do_cmd_options_aux(OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
            do_cmd_options_aux(OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

            screen_load();
            break;
        default:
            bell();
            break;
        }

        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == 6))
            break;
    }

    for (int i = 0; i < A_MAX; i++)
        stat_limit[i] = (s16b)cval[i];

    return TRUE;
}

/*!
 * @brief オートローラで得たい年齢、身長、体重、社会的地位の基準を決める。
 * @return なし
 */
static bool get_chara_limits(player_type* creature_ptr)
{
#define MAXITEMS 8

    char buf[80], cur[80];
    concptr itemname[] = {
        _("年齢", "age"),
        _("身長(インチ)", "height"),
        _("体重(ポンド)", "weight"),
        _("社会的地位", "social class")
    };

    clear_from(10);
    put_str(_("2/4/6/8で項目選択、+/-で値の増減、Enterで次へ",
        "2/4/6/8 for Select, +/- for Change value, Enter for Goto next"), 11, 10);
    put_str(_("注意：身長と体重の最大値/最小値ぎりぎりの値は非常に出現確率が低くなります。",
        "Caution: Values near minimum or maximum are extremely rare."), 23, 2);

    int max_percent, min_percent;
    if (creature_ptr->psex == SEX_MALE) {
        max_percent = (int)(rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1) * 100 / (int)(rp_ptr->m_b_ht);
        min_percent = (int)(rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1) * 100 / (int)(rp_ptr->m_b_ht);
    } else {
        max_percent = (int)(rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1) * 100 / (int)(rp_ptr->f_b_ht);
        min_percent = (int)(rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1) * 100 / (int)(rp_ptr->f_b_ht);
    }

    put_str(_("体格/地位の最小値/最大値を設定して下さい。", "Set minimum/maximum attribute."), 10, 10);
    put_str(_("  項    目                 最小値  最大値", " Parameter                    Min     Max"), 13, 20);
    int mval[MAXITEMS];
    int cval[MAXITEMS];
    for (int i = 0; i < MAXITEMS; i++) {
        int m;
        switch (i) {
        case 0: /* Minimum age */
            m = rp_ptr->b_age + 1;
            break;
        case 1: /* Maximum age */
            m = rp_ptr->b_age + rp_ptr->m_age;
            break;

        case 2: /* Minimum height */
            if (creature_ptr->psex == SEX_MALE)
                m = rp_ptr->m_b_ht - rp_ptr->m_m_ht * 4 + 1;
            else
                m = rp_ptr->f_b_ht - rp_ptr->f_m_ht * 4 + 1;
            break;
        case 3: /* Maximum height */
            if (creature_ptr->psex == SEX_MALE)
                m = rp_ptr->m_b_ht + rp_ptr->m_m_ht * 4 - 1;
            else
                m = rp_ptr->f_b_ht + rp_ptr->f_m_ht * 4 - 1;
            break;
        case 4: /* Minimum weight */
            if (creature_ptr->psex == SEX_MALE)
                m = (rp_ptr->m_b_wt * min_percent / 100) - (rp_ptr->m_m_wt * min_percent / 75) + 1;
            else
                m = (rp_ptr->f_b_wt * min_percent / 100) - (rp_ptr->f_m_wt * min_percent / 75) + 1;
            break;
        case 5: /* Maximum weight */
            if (creature_ptr->psex == SEX_MALE)
                m = (rp_ptr->m_b_wt * max_percent / 100) + (rp_ptr->m_m_wt * max_percent / 75) - 1;
            else
                m = (rp_ptr->f_b_wt * max_percent / 100) + (rp_ptr->f_m_wt * max_percent / 75) - 1;
            break;
        case 6: /* Minimum social class */
            m = 1;
            break;
        case 7: /* Maximum social class */
            m = 100;
            break;
        default:
            m = 1;
            break;
        }

        mval[i] = m;
        cval[i] = m;
    }

    for (int i = 0; i < 4; i++) {
        sprintf(buf, "%-12s (%3d - %3d)", itemname[i], mval[i * 2], mval[i * 2 + 1]);
        put_str(buf, 14 + i, 20);
        for (int j = 0; j < 2; j++) {
            sprintf(buf, "     %3d", cval[i * 2 + j]);
            put_str(buf, 14 + i, 45 + 8 * j);
        }
    }

    int cs = 0;
    int os = MAXITEMS;
    while (TRUE) {
        if (cs != os) {
            const char accept[] = _("決定する", "Accept");
            if (os == MAXITEMS)
                c_put_str(TERM_WHITE, accept, 19, 35);
            else
                c_put_str(TERM_WHITE, cur, 14 + os / 2, 45 + 8 * (os % 2));

            if (cs == MAXITEMS) {
                c_put_str(TERM_YELLOW, accept, 19, 35);
            } else {
                sprintf(cur, "     %3d", cval[cs]);
                c_put_str(TERM_YELLOW, cur, 14 + cs / 2, 45 + 8 * (cs % 2));
            }

            os = cs;
        }

        char c = inkey();
        switch (c) {
        case 'Q':
            birth_quit();
            break;
        case 'S':
            return FALSE;
        case ESCAPE:
            break; /*後でもう一回breakせんと*/
        case ' ':
        case '\r':
        case '\n':
            if (cs == MAXITEMS)
                break;
            cs++;
            c = '6';
            break;
        case '8':
        case 'k':
            if (cs - 2 >= 0)
                cs -= 2;
            break;
        case '2':
        case 'j':
            if (cs < MAXITEMS)
                cs += 2;
            if (cs > MAXITEMS)
                cs = MAXITEMS;
            break;
        case '4':
        case 'h':
            if (cs > 0)
                cs--;
            break;
        case '6':
        case 'l':
            if (cs < MAXITEMS)
                cs++;
            break;
        case '-':
        case '<':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] > cval[cs - 1]) {
                        cval[cs]--;
                        os = 127;
                    }
                } else {
                    if (cval[cs] > mval[cs]) {
                        cval[cs]--;
                        os = 127;
                    }
                }
            }

            break;
        case '+':
        case '>':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] < mval[cs]) {
                        cval[cs]++;
                        os = 127;
                    }
                } else {
                    if (cval[cs] < cval[cs + 1]) {
                        cval[cs]++;
                        os = 127;
                    }
                }
            }

            break;
        case 'm':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] < mval[cs]) {
                        cval[cs] = mval[cs];
                        os = 127;
                    }
                } else {
                    if (cval[cs] < cval[cs + 1]) {
                        cval[cs] = cval[cs + 1];
                        os = 127;
                    }
                }
            }

            break;
        case 'n':
            if (cs != MAXITEMS) {
                if (cs % 2) {
                    if (cval[cs] > cval[cs - 1]) {
                        cval[cs] = cval[cs - 1];
                        os = 255;
                    }
                } else {
                    if (cval[cs] > mval[cs]) {
                        cval[cs] = mval[cs];
                        os = 255;
                    }
                }
            }

            break;
        case '?':
#ifdef JP
            show_help(creature_ptr, "jbirth.txt#AutoRoller");
#else
            show_help(creature_ptr, "birth.txt#AutoRoller");
#endif
            break;
        case '=':
            screen_save();
            do_cmd_options_aux(OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
            break;
        default:
            bell();
            break;
        }

        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == MAXITEMS))
            break;
    }

    chara_limit.agemin = (s16b)cval[0];
    chara_limit.agemax = (s16b)cval[1];
    chara_limit.htmin = (s16b)cval[2];
    chara_limit.htmax = (s16b)cval[3];
    chara_limit.wtmin = (s16b)cval[4];
    chara_limit.wtmax = (s16b)cval[5];
    chara_limit.scmin = (s16b)cval[6];
    chara_limit.scmax = (s16b)cval[7];
    return TRUE;
}

/*!
 * @brief player_birth()関数のサブセット/Helper function for 'player_birth()'
 * @details
 * The delay may be reduced, but is recommended to keep players
 * from continuously rolling up characters, which can be VERY
 * expensive CPU wise.  And it cuts down on player stupidity.
 * @return なし
 */
static bool player_birth_aux(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    BIT_FLAGS mode = 0;
    bool flag = FALSE;
    bool prev = FALSE;
    concptr str;
    char p2 = ')';
    char b1 = '[';
    char b2 = ']';
    char buf[80], cur[80];

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
    int n;
    for (n = 0; n < MAX_SEXES; n++) {
        sp_ptr = &sex_info[n];
        sprintf(buf, _("%c%c%s", "%c%c %s"), I2A(n), p2, sp_ptr->title);
        put_str(buf, 12 + (n / 5), 2 + 15 * (n % 5));
    }

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
                str = sp_ptr->title;
                sprintf(cur, _("%c%c%s", "%c%c %s"), I2A(cs), p2, str);
            }

            c_put_str(TERM_YELLOW, cur, 12 + (cs / 5), 2 + 15 * (cs % 5));
            os = cs;
        }

        if (k >= 0)
            break;

        sprintf(buf, _("性別を選んで下さい (%c-%c) ('='初期オプション設定): ", "Choose a sex (%c-%c) ('=' for options): "), I2A(0), I2A(n - 1));
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

    if (autochara)
        if (!get_chara_limits(creature_ptr))
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

            flag = (!(auto_round % AUTOROLLER_STEP));
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

        mode = 0;
        get_extra(creature_ptr, TRUE);
        get_money(creature_ptr);
        creature_ptr->chaos_patron = (s16b)randint0(MAX_PATRON);
        char c;
        while (TRUE) {
            creature_ptr->update |= (PU_BONUS | PU_HP);
            update_creature(creature_ptr);
            creature_ptr->chp = creature_ptr->mhp;
            creature_ptr->csp = creature_ptr->msp;
            display_player(creature_ptr, mode, map_name);
            Term_gotoxy(2, 23);
            Term_addch(TERM_WHITE, b1);
            Term_addstr(-1, TERM_WHITE, _("'r' 次の数値", "'r'eroll"));
            if (prev)
                Term_addstr(-1, TERM_WHITE, _(", 'p' 前の数値", "'p'previous"));

            if (mode)
                Term_addstr(-1, TERM_WHITE, _(", 'h' その他の情報", ", 'h' Misc."));
            else
                Term_addstr(-1, TERM_WHITE, _(", 'h' 生い立ちを表示", ", 'h'istory"));

            Term_addstr(-1, TERM_WHITE, _(", Enter この数値に決定", ", or Enter to accept"));
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

/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 * @return なし
 */
void player_birth(player_type* creature_ptr, void (*process_autopick_file_command)(char*))
{
    current_world_ptr->play_time = 0;
    wipe_monsters_list(creature_ptr);
    player_wipe_without_name(creature_ptr);
    if (!ask_quick_start(creature_ptr)) {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DEFAULT);
        while (TRUE) {
            if (player_birth_aux(creature_ptr, process_autopick_file_command))
                break;

            player_wipe_without_name(creature_ptr);
        }
    }

    message_add(" ");
    message_add("  ");
    message_add("====================");
    message_add(" ");
    message_add("  ");

    exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(creature_ptr, DIARY_DIALY, 0, NULL);
    char buf[80];
    sprintf(buf, _("                            性別に%sを選択した。", "                            chose %s gender."),
        sex_info[creature_ptr->psex].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("                            種族に%sを選択した。", "                            chose %s race."),
        race_info[creature_ptr->prace].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("                            職業に%sを選択した。", "                            chose %s class."),
        class_info[creature_ptr->pclass].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    if (creature_ptr->realm1) {
        sprintf(buf, _("                            魔法の領域に%s%sを選択した。", "                            chose %s%s."),
            realm_names[creature_ptr->realm1], creature_ptr->realm2 ? format(_("と%s", " and %s realms"), realm_names[creature_ptr->realm2]) : _("", " realm"));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    }

    sprintf(buf, _("                            性格に%sを選択した。", "                            chose %s personality."),
        seikaku_info[creature_ptr->pseikaku].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);

    for (int i = 1; i < max_towns; i++) {
        for (int j = 0; j < MAX_STORES; j++) {
            store_init(i, j);
        }
    }

    seed_wilderness();
    if (creature_ptr->prace == RACE_BEASTMAN)
        creature_ptr->hack_mutation = TRUE;
    else
        creature_ptr->hack_mutation = FALSE;

    if (!window_flag[1])
        window_flag[1] |= PW_MESSAGE;

    if (!window_flag[2])
        window_flag[2] |= PW_INVEN;
}
