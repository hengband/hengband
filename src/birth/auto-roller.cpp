#include "birth/auto-roller.h"
#include "birth/birth-stat.h"
#include "birth/birth-util.h"
#include "cmd-io/cmd-gameoption.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race.h"
#include "player/player-sex.h"
#include "player/player-status-table.h"
#include "system/game-option-types.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"

/*! オートローラの能力値的要求水準 / Autoroll limit */
int16_t stat_limit[6];

/*! オートローラの試行回数 / Autoroll round */
int32_t auto_round;
int32_t auto_upper_round;

/*! オートローラの要求値実現確率 */
int32_t autoroll_chance;

/*!
 * @breif オートローラーで指定した能力値以上が出る確率を計算する。
 * @return 確率 / 100
 */
static int32_t get_autoroller_prob(int *minval)
{
    /* 1 percent of the valid random space (60^6 && 72<sum<87) */
    int32_t tot_rand_1p = 320669745;
    int i, j, tmp;
    int ii[6];
    int tval[6];
    int tot = 0;

    /* success count */
    int32_t succ = 0;

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
 * @brief オートローラの初期設定値を決定する
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param cval 設定能力値配列
 * @details
 * 純戦士系及び腕器耐が魔法の能力の職業は腕器耐17。
 * デュアルは腕耐17で器と魔法の能力が16。
 * 純メイジ系は耐と魔法の能力が17で腕器16。
 * デュアルかどうかは最大攻撃回数で決定。(4回以上)
 */
static void decide_initial_stat(player_type *creature_ptr, int *cval)
{
    auto &class_ptr = class_info[creature_ptr->pclass];
    auto &magic_ptr = m_info[creature_ptr->pclass];
    auto is_magic_user = magic_ptr.spell_stat == A_INT || magic_ptr.spell_stat == A_WIS || magic_ptr.spell_stat == A_CHR;
    auto is_attacker = class_ptr.num > 3;

    auto num_17 = 0;
    if (is_magic_user) {
        auto st = magic_ptr.spell_stat;
        if (st >= 0 && st < A_MAX) {
            if (is_attacker)
                cval[st] = 16;
            else {
                cval[st] = 17;
                num_17++;
            }
        }
    }

    if (cval[A_CON] == 0) {
        cval[A_CON] = 17;
        if (is_magic_user)
            num_17++;
    }

    if (cval[A_STR] == 0) {
        cval[A_STR] = num_17 == 2 ? 16 : 17;
        if (is_magic_user && num_17 < 2)
            num_17++;
    }

    if (cval[A_DEX] == 0)
        cval[A_DEX] = 17 - MAX(0, num_17 - 1);

    for (int i = 0; i < A_MAX; i++)
        if (cval[i] == 0)
            cval[i] = 8;
}

/*!
 * @brief オートローラの設定能力値行を作成する
 * @param cur カーソル文字列を入れるバッファ
 * @param cval 設定能力値配列
 * @param cs カーソル位置(能力値番号)
 */
static void cursor_of_adjusted_stat(char *cur, int *cval, int cs)
{
    char inp[80], maxv[80];
    auto j = rp_ptr->r_adj[cs] + cp_ptr->c_adj[cs] + ap_ptr->a_adj[cs];
    auto m = adjust_stat(17, j);
    if (m > 18)
        sprintf(maxv, "18/%02d", (m - 18));
    else
        sprintf(maxv, "%2d", m);

    m = adjust_stat(cval[cs], j);
    if (m > 18)
        sprintf(inp, "18/%02d", (m - 18));
    else
        sprintf(inp, "%2d", m);

    sprintf(cur, "%6s       %2d   %+3d  %+3d  %+3d  =  %6s  %6s", stat_names[cs], cval[cs], rp_ptr->r_adj[cs], cp_ptr->c_adj[cs], ap_ptr->a_adj[cs], inp, maxv);
}

/*!
 * @brief オートローラの確率を表示
 * @param cval 設定能力値配列
 */
static void display_autoroller_chance(int *cval)
{
    char buf[320];
    autoroll_chance = get_autoroller_prob(cval);
    if (autoroll_chance == -999)
        sprintf(buf, _("確率: 不可能(合計86超)       ", "Prob: Impossible(>86 tot stats)"));
    else if (autoroll_chance < 1)
        sprintf(buf, _("確率: 非常に容易(1/10000以上)", "Prob: Quite Easy(>1/10000)     "));
    else
        sprintf(buf, _("確率: 約 1/%8d00             ", "Prob: ~ 1/%8d00                "), autoroll_chance);
    put_str(buf, 23, 25);
}

/*!
 * @brief オートローラで得たい能力値の基準を決める。
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
bool get_stat_limits(player_type *creature_ptr)
{
    clear_from(10);
    put_str(_("能力値を抽選します。最低限得たい能力値を設定して下さい。", "Set minimum stats for picking up your charactor."), 10, 10);
    put_str(_("2/8で項目選択、4/6で値の増減、Enterで次へ", "2/8 for Select, 4/6 for Change value, Enter for Goto next"), 11, 10);
    put_str(_("         基本値  種族 職業 性格     合計値  最大値", "           Base   Rac  Cla  Per      Total  Maximum"), 13, 10);

    int cval[A_MAX]{};
    decide_initial_stat(creature_ptr, cval);

    char buf[320];
    char cur[160];
    for (int i = 0; i < A_MAX; i++) {
        cursor_of_adjusted_stat(buf, cval, i);
        put_str(buf, 14 + i, 10);
    }

    display_autoroller_chance(cval);

    int cs = 0;
    int os = A_MAX;
    while (true) {
        if (cs != os) {
            if (os == 7)
                display_autoroller_chance(cval);
            else if (os == A_MAX)
                c_put_str(TERM_WHITE, _("決定する", "Accept"), 21, 35);
            else if (os < A_MAX)
                c_put_str(TERM_WHITE, cur, 14 + os, 10);

            if (cs == A_MAX)
                c_put_str(TERM_YELLOW, _("決定する", "Accept"), 21, 35);
            else {
                cursor_of_adjusted_stat(cur, cval, cs);
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
            return false;
        case ESCAPE:
            break;
        case ' ':
        case '\r':
        case '\n':
            if (cs == A_MAX)
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
            if (cs != A_MAX) {
                if (cval[cs] == 3) {
                    cval[cs] = 17;
                    os = 7;
                } else if (cval[cs] > 3) {
                    cval[cs]--;
                    os = 7;
                } else
                    return false;
            }

            break;
        case '6':
        case 'l':
            if (cs != A_MAX) {
                if (cval[cs] == 17) {
                    cval[cs] = 3;
                    os = 7;
                } else if (cval[cs] < 17) {
                    cval[cs]++;
                    os = 7;
                } else
                    return false;
            }

            break;
        case 'm':
            if (cs != A_MAX) {
                cval[cs] = 17;
                os = 7;
            }

            break;
        case 'n':
            if (cs != A_MAX) {
                cval[cs] = 3;
                os = 7;
            }

            break;
        case '?':
            show_help(creature_ptr, _("jbirth.txt#AutoRoller", "birth.txt#AutoRoller"));
            break;
        case '=':
            screen_save();
            do_cmd_options_aux(creature_ptr, OPT_PAGE_BIRTH,
                _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
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
        stat_limit[i] = (int16_t)cval[i];

    return true;
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
    while (true) {
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
            return false;
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
            do_cmd_options_aux(creature_ptr, OPT_PAGE_BIRTH, _("初期オプション((*)はスコアに影響)", "Birth Options ((*)) affect score"));
            screen_load();
            break;
        default:
            bell();
            break;
        }

        if (c == ESCAPE || ((c == ' ' || c == '\r' || c == '\n') && cs == MAXITEMS))
            break;
    }

    chara_limit_ptr->agemin = (int16_t)cval[0];
    chara_limit_ptr->agemax = (int16_t)cval[1];
    chara_limit_ptr->htmin = (int16_t)cval[2];
    chara_limit_ptr->htmax = (int16_t)cval[3];
    chara_limit_ptr->wtmin = (int16_t)cval[4];
    chara_limit_ptr->wtmax = (int16_t)cval[5];
    chara_limit_ptr->scmin = (int16_t)cval[6];
    chara_limit_ptr->scmax = (int16_t)cval[7];
    return true;
}
