#include "birth/auto-roller.h"
#include "birth/birth-stat.h"
#include "birth/birth-util.h"
#include "cmd-io/cmd-gameoption.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race.h"
#include "player/player-status-table.h"
#include "system/game-option-types.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"

/*! オートローラの能力値的要求水準 / Autoroll limit */
s16b stat_limit[6];

/*! オートローラの試行回数 / Autoroll round */
s32b auto_round;
s32b auto_upper_round;

/*! オートローラの要求値実現確率 */
s32b autoroll_chance;

/*!
 * @breif オートローラーで指定した能力値以上が出る確率を計算する。
 * @return 確率 / 100
 */
static s32b get_autoroller_prob(int *minval)
{
    /* 1 percent of the valid random space (60^6 && 72<sum<87) */
    s32b tot_rand_1p = 320669745;
    int i, j, tmp;
    int ii[6];
    int tval[6];
    int tot = 0;

    /* success count */
    s32b succ = 0;

    /* random combinations out of 60 (1d3+1d4+1d5) patterns */
    int pp[18] = {
        0, 0, 0, 0, 0, 0, 0, 0, /* 0-7 */
        1, 3, 6, 9, 11, 11, 9, 6, 3, 1 /* 8-17 */
    };

    /* Copy */
    for (i = 0; i < 6; i++) {
        tval[i] = MAX(8, minval[i]);
        tot += tval[i];
    }

    /* No Chance */
    if (tot > 86)
        return -999;

    /* bubble sort for speed-up */
    for (i = 0; i < 5; i++) {
        for (j = 5; j > i; j--) {
            if (tval[j - 1] < tval[j]) {
                tmp = tval[j - 1];
                tval[j - 1] = tval[j];
                tval[j] = tmp;
            }
        }
    }

    tot = 0;

    /* calc. prob. */
    for (ii[0] = tval[0]; ii[0] < 18; ii[0]++) {
        for (ii[1] = tval[1]; ii[1] < 18; ii[1]++) {
            for (ii[2] = tval[2]; ii[2] < 18; ii[2]++) {
                for (ii[3] = tval[3]; ii[3] < 18; ii[3]++) {
                    for (ii[4] = tval[4]; ii[4] < 18; ii[4]++) {
                        for (ii[5] = tval[5]; ii[5] < 18; ii[5]++) {
                            tot = ii[0] + ii[1] + ii[2] + ii[3] + ii[4] + ii[5];

                            if (tot > 86)
                                break;
                            if (tot <= 72)
                                continue;

                            succ += (pp[ii[0]] * pp[ii[1]] * pp[ii[2]] * pp[ii[3]] * pp[ii[4]] * pp[ii[5]]);

                            /* If given condition is easy enough, quit calc. to save CPU. */
                            if (succ > 320670)
                                return -1;
                        }
                    }
                }
            }
        }
    }

    return tot_rand_1p / succ;
}

/*!
 * @brief オートローラで得たい能力値の基準を決める。
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
bool get_stat_limits(player_type *creature_ptr)
{
    int col_stat = 25;

    clear_from(10);
    put_str(_("最低限得たい能力値を設定して下さい。", "Set minimum stats."), 10, 10);
    put_str(_("2/8で項目選択、4/6で値の増減、Enterで次へ", "2/8 for Select, 4/6 for Change value, Enter for Goto next"), 11, 10);
    put_str(_("         基本値  種族 職業 性格     合計値  最大値", "           Base   Rac  Cla  Per      Total  Maximum"), 13, 10);

    put_str(_("確率: 非常に容易(1/10000以上)", "Prob: Quite Easy(>1/10000)"), 23, col_stat);

    int cval[6];
    char buf[80];
    char cur[80];
    char inp[80];
    for (int i = 0; i < A_MAX; i++) {
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

        sprintf(buf, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s  %6s", stat_names[i], cval[i], rp_ptr->r_adj[i], cp_ptr->c_adj[i], ap_ptr->a_adj[i], inp, cur);
        put_str(buf, 14 + i, 10);
    }

    int cs = 0;
    int os = 6;
    while (TRUE) {
        if (cs != os) {
            if (os == 7) {
                autoroll_chance = get_autoroller_prob(cval);
                if (autoroll_chance == -999)
                    sprintf(buf, _("確率: 不可能(合計86超)       ", "Prob: Impossible(>86 tot stats)"));
                else if (autoroll_chance < 1)
                    sprintf(buf, _("確率: 非常に容易(1/10000以上)", "Prob: Quite Easy(>1/10000)     "));
                else
                    sprintf(buf, _("確率: 約 1/%8d00          ", "Prob: ~ 1/%8d00            "), autoroll_chance);
                put_str(buf, 23, col_stat);
            }
            else if (os == 6) {
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

                sprintf(
                    cur, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s", stat_names[cs], cval[cs], rp_ptr->r_adj[cs], cp_ptr->c_adj[cs], ap_ptr->a_adj[cs], inp);
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
            do_cmd_options_aux(creature_ptr, OPT_PAGE_BIRTH, "初期オプション((*)はスコアに影響)");
#else
            do_cmd_options_aux(creature_ptr, OPT_PAGE_BIRTH, "Birth Option((*)s effect score)");
#endif

            screen_load();
            break;
        default:
            bell();
            break;
        }

        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == 6 && autoroll_chance != -999))
            break;
    }

    for (int i = 0; i < A_MAX; i++)
        stat_limit[i] = (s16b)cval[i];

    return TRUE;
}

void initialize_chara_limit(chara_limit_type *chara_limit_ptr)
{
    chara_limit_ptr->agemin = 0;
    chara_limit_ptr->agemax = 0;
    chara_limit_ptr->htmin = 0;
    chara_limit_ptr->htmax = 0;
    chara_limit_ptr->wtmin = 0;
    chara_limit_ptr->wtmax = 0;
    chara_limit_ptr->scmin = 0;
    chara_limit_ptr->scmax = 0;
}

/*!
 * @brief オートローラで得たい年齢、身長、体重、社会的地位の基準を決める。
 * @return なし
 */
bool get_chara_limits(player_type *creature_ptr, chara_limit_type *chara_limit_ptr)
{
#define MAXITEMS 8

    char buf[80], cur[80];
    concptr itemname[] = { _("年齢", "age"), _("身長(インチ)", "height"), _("体重(ポンド)", "weight"), _("社会的地位", "social class") };

    clear_from(10);
    put_str(_("2/4/6/8で項目選択、+/-で値の増減、Enterで次へ", "2/4/6/8 for Select, +/- for Change value, Enter for Goto next"), 11, 10);
    put_str(
        _("注意：身長と体重の最大値/最小値ぎりぎりの値は非常に出現確率が低くなります。", "Caution: Values near minimum or maximum are extremely rare."), 23, 2);

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
            do_cmd_options_aux(creature_ptr, OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Option((*)s effect score)"));
            screen_load();
            break;
        default:
            bell();
            break;
        }

        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == MAXITEMS))
            break;
    }

    chara_limit_ptr->agemin = (s16b)cval[0];
    chara_limit_ptr->agemax = (s16b)cval[1];
    chara_limit_ptr->htmin = (s16b)cval[2];
    chara_limit_ptr->htmax = (s16b)cval[3];
    chara_limit_ptr->wtmin = (s16b)cval[4];
    chara_limit_ptr->wtmax = (s16b)cval[5];
    chara_limit_ptr->scmin = (s16b)cval[6];
    chara_limit_ptr->scmax = (s16b)cval[7];
    return TRUE;
}
