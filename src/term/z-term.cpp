/*
 * @brief Purpose: a generic, efficient, terminal window package -BEN-
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#include "term/z-term.h"
#include "game-option/map-screen-options.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "term/z-virt.h"

/* Special flags in the attr data */
#define AF_BIGTILE2 0xf0
#define AF_TILE1 0x80

#ifdef JP
/*
 * 全角文字対応。
 * 属性に全角文字の1バイト目、2バイト目も記憶。
 * By FIRST
 */
#define AF_KANJI1 0x10
#define AF_KANJI2 0x20
#define AF_KANJIC 0x0f
#endif

/* The current "term" */
term_type *Term = nullptr;

/*** Local routines ***/

/*
 * Initialize a "term_win" (using the given window size)
 */
term_win::term_win(TERM_LEN w, TERM_LEN h)
    : a(h, std::vector<TERM_COLOR>(w))
    , c(h, std::vector<char>(w))
    , ta(h, std::vector<TERM_COLOR>(w))
    , tc(h, std::vector<char>(w))
{
}

std::unique_ptr<term_win> term_win::create(TERM_LEN w, TERM_LEN h)
{
    // privateコンストラクタを呼び出すための補助クラス
    struct impl : term_win {
        impl(TERM_LEN w, TERM_LEN h)
            : term_win(w, h)
        {
        }
    };
    return std::make_unique<impl>(w, h);
}

std::unique_ptr<term_win> term_win::clone() const
{
    return std::make_unique<term_win>(*this);
}

void term_win::resize(TERM_LEN w, TERM_LEN h)
{
    /* Ignore non-changes */
    if (this->a.size() == static_cast<size_t>(h) && this->a[0].size() == static_cast<size_t>(w))
        return;

    this->a.resize(h, std::vector<TERM_COLOR>(w));
    this->c.resize(h, std::vector<char>(w));
    this->ta.resize(h, std::vector<TERM_COLOR>(w));
    this->tc.resize(h, std::vector<char>(w));

    for (TERM_LEN y = 0; y < h; y++) {
        this->a[y].resize(w);
        this->c[y].resize(w);
        this->ta[y].resize(w);
        this->tc[y].resize(w);
    }

    /* Illegal cursor */
    if (this->cx >= w)
        this->cu = 1;
    if (this->cy >= h)
        this->cu = 1;
}

/*** External hooks ***/

/*
 * Execute the "Term->user_hook" hook, if available (see above).
 */
errr term_user(int n)
{
    /* Verify the hook */
    if (!Term->user_hook)
        return -1;

    /* Call the hook */
    return ((*Term->user_hook)(n));
}

/*
 * Execute the "Term->xtra_hook" hook, if available (see above).
 */
errr term_xtra(int n, int v)
{
    /* Verify the hook */
    if (!Term->xtra_hook)
        return -1;

    /* Call the hook */
    return ((*Term->xtra_hook)(n, v));
}

/*** Fake hooks ***/

/*
 * Fake hook for "term_curs()" (see above)
 */
static errr term_curs_hack(TERM_LEN x, TERM_LEN y)
{
    /* Unused */
    (void)x;
    (void)y;

    return -1;
}

/*
 * Fake hook for "term_bigcurs()" (see above)
 */
static errr term_bigcurs_hack(TERM_LEN x, TERM_LEN y)
{
    return (*Term->curs_hook)(x, y);
}

/*
 * Fake hook for "term_wipe()" (see above)
 */
static errr term_wipe_hack(TERM_LEN x, TERM_LEN y, int n)
{
    /* Unused */
    (void)x;
    (void)y;
    (void)n;

    return -1;
}

/*
 * Fake hook for "term_text()" (see above)
 */
static errr term_text_hack(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr cp)
{
    /* Unused */
    (void)x;
    (void)y;
    (void)n;
    (void)a;
    (void)cp;

    return -1;
}

/*
 * Fake hook for "term_pict()" (see above)
 */
static errr term_pict_hack(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, concptr cp, const TERM_COLOR *tap, concptr tcp)
{
    /* Unused */
    (void)x;
    (void)y;
    (void)n;
    (void)ap;
    (void)cp;
    (void)tap;
    (void)tcp;

    return -1;
}

/*** Efficient routines ***/

/*
 * Mentally draw an attr/char at a given location
 * Assumes given location and values are valid.
 */
void term_queue_char(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c, TERM_COLOR ta, char tc)
{
    const auto &scrn = Term->scr;

    TERM_COLOR *scr_aa = &scrn->a[y][x];
    char *scr_cc = &scrn->c[y][x];

    TERM_COLOR *scr_taa = &scrn->ta[y][x];
    char *scr_tcc = &scrn->tc[y][x];

    /* Ignore non-changes */
    if ((*scr_aa == a) && (*scr_cc == c) && (*scr_taa == ta) && (*scr_tcc == tc))
        return;

    /* Save the "literal" information */
    *scr_aa = a;
    *scr_cc = c;

    *scr_taa = ta;
    *scr_tcc = tc;

    /* Check for new min/max row info */
    if (y < Term->y1)
        Term->y1 = y;
    if (y > Term->y2)
        Term->y2 = y;

    /* Check for new min/max col info for this row */
    if (x < Term->x1[y])
        Term->x1[y] = x;
    if (x > Term->x2[y])
        Term->x2[y] = x;

#ifdef JP
    if (((scrn->a[y][x] & AF_BIGTILE2) == AF_BIGTILE2) || (scrn->a[y][x] & AF_KANJI2))
#else
    if ((scrn->a[y][x] & AF_BIGTILE2) == AF_BIGTILE2)
#endif
        if ((x - 1) < Term->x1[y])
            Term->x1[y]--;
}

/*
 * Bigtile version of term_queue_char().
 * If use_bigtile is FALSE, simply call term_queue_char().
 * Otherwise, mentally draw a pair of attr/char at a given location.
 * Assumes given location and values are valid.
 */
void term_queue_bigchar(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c, TERM_COLOR ta, char tc)
{
#ifdef JP
    /*
     * A table which relates each ascii character to a multibyte
     * character.
     *
     * 「■」は二倍幅豆腐の内部コードに使用。
     */
    static char ascii_to_zenkaku[] = "　！”＃＄％＆’（）＊＋，－．／"
                                     "０１２３４５６７８９：；＜＝＞？"
                                     "＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯ"
                                     "ＰＱＲＳＴＵＶＷＸＹＺ［＼］＾＿"
                                     "‘ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏ"
                                     "ｐｑｒｓｔｕｖｗｘｙｚ｛｜｝～■";
#endif

    byte a2;
    char c2;

    /* If non bigtile mode, call orginal function */
    if (!use_bigtile) {
        term_queue_char(x, y, a, c, ta, tc);
        return;
    }

    /* A tile becomes a Bigtile */
    if ((a & AF_TILE1) && (c & 0x80)) {
        /* Mark it as a Bigtile */
        a2 = AF_BIGTILE2;

        c2 = -1;

        /* Ignore non-tile background */
        if (!((ta & AF_TILE1) && (tc & 0x80))) {
            ta = 0;
            tc = 0;
        }
    }

#ifdef JP
    /*
     * Use a multibyte character instead of a dirty pair of ASCII
     * characters.
     */
    else if (' ' <= c) /* isprint(c) */
    {
        c2 = ascii_to_zenkaku[2 * (c - ' ') + 1];
        c = ascii_to_zenkaku[2 * (c - ' ')];

        /* Mark it as a Kanji */
        a2 = a | AF_KANJI2;
        a |= AF_KANJI1;
    }
#endif

    else {
        /* Dirty pair of ASCII characters */
        a2 = TERM_WHITE;
        c2 = ' ';
    }

    /* Display pair of attr/char */
    term_queue_char(x, y, a, c, ta, tc);
    term_queue_char(x + 1, y, a2, c2, 0, 0);
}

/*
 * Mentally draw a string of attr/chars at a given location
 * Assumes given location and values are valid.
 * This function is designed to be fast, with no consistancy checking.
 * It is used to update the map in the game.
 */
void term_queue_line(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR *a, char *c, TERM_COLOR *ta, char *tc)
{
    const auto &scrn = Term->scr;

    TERM_LEN x1 = -1;
    TERM_LEN x2 = -1;

    TERM_COLOR *scr_aa = &scrn->a[y][x];
    char *scr_cc = &scrn->c[y][x];

    TERM_COLOR *scr_taa = &scrn->ta[y][x];
    char *scr_tcc = &scrn->tc[y][x];

    while (n--) {
        /* Ignore non-changes */
        if ((*scr_aa == *a) && (*scr_cc == *c) && (*scr_taa == *ta) && (*scr_tcc == *tc)) {
            x++;
            a++;
            c++;
            ta++;
            tc++;
            scr_aa++;
            scr_cc++;
            scr_taa++;
            scr_tcc++;
            continue;
        }

        /* Save the "literal" information */
        *scr_taa++ = *ta++;
        *scr_tcc++ = *tc++;

        /* Save the "literal" information */
        *scr_aa++ = *a++;
        *scr_cc++ = *c++;

        /* Track minimum changed column */
        if (x1 < 0)
            x1 = x;

        /* Track maximum changed column */
        x2 = x;

        x++;
    }

    /* Expand the "change area" as needed */
    if (x1 >= 0) {
        /* Check for new min/max row info */
        if (y < Term->y1)
            Term->y1 = y;
        if (y > Term->y2)
            Term->y2 = y;

        /* Check for new min/max col info in this row */
        if (x1 < Term->x1[y])
            Term->x1[y] = x1;
        if (x2 > Term->x2[y])
            Term->x2[y] = x2;
    }
}

/*
 * Mentally draw some attr/chars at a given location
 *
 * Assumes that (x,y) is a valid location, that the first "n" characters
 * of the string "s" are all valid (non-zero), and that (x+n-1,y) is also
 * a valid location, so the first "n" characters of "s" can all be added
 * starting at (x,y) without causing any illegal operations.
 */
static void term_queue_chars(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr s)
{
    TERM_LEN x1 = -1, x2 = -1;

    auto &scr_aa = Term->scr->a[y];
#ifdef JP
    auto &scr_cc = Term->scr->c[y];

    auto &scr_taa = Term->scr->ta[y];
    auto &scr_tcc = Term->scr->tc[y];
#else
    auto &scr_cc = Term->scr->c[y];

    auto &scr_taa = Term->scr->ta[y];
    auto &scr_tcc = Term->scr->tc[y];
#endif

#ifdef JP
    /* 表示文字なし */
    if (n == 0 || *s == 0)
        return;
    /*
     * 全角文字の右半分から文字を表示する場合、
     * 重なった文字の左部分を消去。
     * 表示開始位置が左端でないと仮定。
     */
    if ((scr_aa[x] & AF_KANJI2) && (scr_aa[x] & AF_BIGTILE2) != AF_BIGTILE2) {
        scr_cc[x - 1] = ' ';
        scr_aa[x - 1] &= AF_KANJIC;
        x1 = x2 = x - 1;
    }
#endif
    /* Queue the attr/chars */
    for (; n; x++, s++, n--) {
#ifdef JP
        /* 特殊文字としてMSBが立っている可能性がある */
        /* その場合attrのMSBも立っているのでこれで識別する */
        /* check */
        if (!(a & AF_TILE1) && iskanji(*s)) {
            char nc1 = *s++;
            char nc2 = *s;

            byte na1 = (a | AF_KANJI1);
            byte na2 = (a | AF_KANJI2);

            if ((--n == 0) || !nc2)
                break;

            if (scr_aa[x++] == na1 && scr_aa[x] == na2 && scr_cc[x - 1] == nc1 && scr_cc[x] == nc2 && (scr_taa[x - 1] == 0) && (scr_taa[x] == 0)
                && (scr_tcc[x - 1] == 0) && (scr_tcc[x] == 0))
                continue;

            scr_aa[x - 1] = na1;
            scr_aa[x] = na2;
            scr_cc[x - 1] = nc1;
            scr_cc[x] = nc2;

            if (x1 < 0)
                x1 = x - 1;
            x2 = x;
        } else {
#endif
            TERM_COLOR oa = scr_aa[x];
            char oc = scr_cc[x];

            TERM_COLOR ota = scr_taa[x];
            char otc = scr_tcc[x];

            /* Ignore non-changes */
            if ((oa == a) && (oc == *s) && (ota == 0) && (otc == 0))
                continue;

            /* Save the "literal" information */
            scr_aa[x] = a;
            scr_cc[x] = *s;

            scr_taa[x] = 0;
            scr_tcc[x] = 0;

            /* Note the "range" of window updates */
            if (x1 < 0)
                x1 = x;
            x2 = x;
#ifdef JP
        }
#endif
    }

#ifdef JP
    /*
     * 全角文字の左半分で表示を終了する場合、
     * 重なった文字の右部分を消去。
     * (条件追加：タイルの1文字目でない事を確かめるように。)
     */
    {
        int w, h;
        term_get_size(&w, &h);
        if (x != w && !(scr_aa[x] & AF_TILE1) && (scr_aa[x] & AF_KANJI2)) {
            scr_cc[x] = ' ';
            scr_aa[x] &= AF_KANJIC;
            if (x1 < 0)
                x1 = x;
            x2 = x;
        }
    }
#endif
    /* Expand the "change area" as needed */
    if (x1 >= 0) {
        /* Check for new min/max row info */
        if (y < Term->y1)
            Term->y1 = y;
        if (y > Term->y2)
            Term->y2 = y;

        /* Check for new min/max col info in this row */
        if (x1 < Term->x1[y])
            Term->x1[y] = x1;
        if (x2 > Term->x2[y])
            Term->x2[y] = x2;
    }
}

/*** Refresh routines ***/

/*
 * Flush a row of the current window (see "term_fresh")
 * Display text using "term_pict()"
 */
static void term_fresh_row_pict(TERM_LEN y, TERM_LEN x1, TERM_LEN x2)
{
    auto &old_aa = Term->old->a[y];
    auto &old_cc = Term->old->c[y];

    const auto &scr_aa = Term->scr->a[y];
    const auto &scr_cc = Term->scr->c[y];

    auto &old_taa = Term->old->ta[y];
    auto &old_tcc = Term->old->tc[y];

    const auto &scr_taa = Term->scr->ta[y];
    const auto &scr_tcc = Term->scr->tc[y];

    TERM_COLOR ota;
    char otc;

    TERM_COLOR nta;
    char ntc;

    /* Pending length */
    TERM_LEN fn = 0;

    /* Pending start */
    TERM_LEN fx = 0;

    TERM_COLOR oa;
    char oc;

    TERM_COLOR na;
    char nc;

#ifdef JP
    /* 全角文字の２バイト目かどうか */
    int kanji = 0;
#endif
    /* Scan "modified" columns */
    for (TERM_LEN x = x1; x <= x2; x++) {
        /* See what is currently here */
        oa = old_aa[x];
        oc = old_cc[x];

        /* See what is desired there */
        na = scr_aa[x];
        nc = scr_cc[x];

#ifdef JP
        if (kanji) {
            /* 全角文字２バイト目 */
            kanji = 0;
            old_aa[x] = na;
            old_cc[x] = nc;
            fn++;
            continue;
        }
        /* 特殊文字としてMSBが立っている可能性がある */
        /* その場合attrのMSBも立っているのでこれで識別する */
        /* check */
        kanji = (iskanji(nc) && !(na & AF_TILE1));
#endif

        ota = old_taa[x];
        otc = old_tcc[x];

        nta = scr_taa[x];
        ntc = scr_tcc[x];

        /* Handle unchanged grids */
#ifdef JP
        if ((na == oa) && (nc == oc) && (nta == ota) && (ntc == otc)
            && (!kanji
                || (scr_aa[x + 1] == old_aa[x + 1] && scr_cc[x + 1] == old_cc[x + 1] && scr_taa[x + 1] == old_taa[x + 1] && scr_tcc[x + 1] == old_tcc[x + 1])))
#else
        if ((na == oa) && (nc == oc) && (nta == ota) && (ntc == otc))
#endif
        {
            /* Flush */
            if (fn) {
                /* Draw pending attr/char pairs */
                (void)((*Term->pict_hook)(fx, y, fn, &scr_aa[fx], &scr_cc[fx], &scr_taa[fx], &scr_tcc[fx]));

                /* Forget */
                fn = 0;
            }

#ifdef JP
            /* 全角文字の時は再開位置は＋１ */
            if (kanji) {
                x++;
                fx++;
                kanji = 0;
            }
#endif
            /* Skip */
            continue;
        }
        /* Save new contents */
        old_aa[x] = na;
        old_cc[x] = nc;

        old_taa[x] = nta;
        old_tcc[x] = ntc;

        /* Restart and Advance */
        if (fn++ == 0)
            fx = x;
    }

    /* Flush */
    if (fn) {
        /* Draw pending attr/char pairs */
        (void)((*Term->pict_hook)(fx, y, fn, &scr_aa[fx], &scr_cc[fx], &scr_taa[fx], &scr_tcc[fx]));
    }
}

/*
 * Flush a row of the current window (see "term_fresh")
 *
 * Display text using "term_text()" and "term_wipe()",
 * but use "term_pict()" for high-bit attr/char pairs
 */
static void term_fresh_row_both(TERM_LEN y, int x1, int x2)
{
    auto &old_aa = Term->old->a[y];
    auto &old_cc = Term->old->c[y];

    const auto &scr_aa = Term->scr->a[y];
    const auto &scr_cc = Term->scr->c[y];

    auto &old_taa = Term->old->ta[y];
    auto &old_tcc = Term->old->tc[y];
    const auto &scr_taa = Term->scr->ta[y];
    const auto &scr_tcc = Term->scr->tc[y];

    TERM_COLOR ota;
    char otc;
    TERM_COLOR nta;
    char ntc;

    /* The "always_text" flag */
    int always_text = Term->always_text;

    /* Pending length */
    int fn = 0;

    /* Pending start */
    int fx = 0;

    /* Pending attr */
    byte fa = Term->attr_blank;

    TERM_COLOR oa;
    char oc;

    TERM_COLOR na;
    char nc;

#ifdef JP
    /* 全角文字の２バイト目かどうか */
    int kanji = 0;
#endif
    /* Scan "modified" columns */
    for (TERM_LEN x = x1; x <= x2; x++) {
        /* See what is currently here */
        oa = old_aa[x];
        oc = old_cc[x];

        /* See what is desired there */
        na = scr_aa[x];
        nc = scr_cc[x];

#ifdef JP
        if (kanji) {
            /* 全角文字２バイト目 */
            kanji = 0;
            old_aa[x] = na;
            old_cc[x] = nc;
            fn++;
            continue;
        }
        /* 特殊文字としてMSBが立っている可能性がある */
        /* その場合attrのMSBも立っているのでこれで識別する */
        /* check */
        /*		kanji = (iskanji(nc));  */
        kanji = (iskanji(nc) && !(na & AF_TILE1));
#endif

        ota = old_taa[x];
        otc = old_tcc[x];

        nta = scr_taa[x];
        ntc = scr_tcc[x];

        /* Handle unchanged grids */
#ifdef JP
        if ((na == oa) && (nc == oc) && (nta == ota) && (ntc == otc)
            && (!kanji
                || (scr_aa[x + 1] == old_aa[x + 1] && scr_cc[x + 1] == old_cc[x + 1] && scr_taa[x + 1] == old_taa[x + 1] && scr_tcc[x + 1] == old_tcc[x + 1])))
#else
        if ((na == oa) && (nc == oc) && (nta == ota) && (ntc == otc))
#endif
        {
            /* Flush */
            if (fn) {
                /* Draw pending chars (normal) */
                if (fa || always_text) {
                    (void)((*Term->text_hook)(fx, y, fn, fa, &scr_cc[fx]));
                }

                /* Draw pending chars (black) */
                else {
                    (void)((*Term->wipe_hook)(fx, y, fn));
                }

                /* Forget */
                fn = 0;
            }

#ifdef JP
            /* 全角文字の時は再開位置は＋１ */
            if (kanji) {
                x++;
                fx++;
                kanji = 0;
            }
#endif
            /* Skip */
            continue;
        }

        /* Save new contents */
        old_aa[x] = na;
        old_cc[x] = nc;

        old_taa[x] = nta;
        old_tcc[x] = ntc;

        /* 2nd byte of bigtile */
        if ((na & AF_BIGTILE2) == AF_BIGTILE2)
            continue;

        /* Handle high-bit attr/chars */
        if ((na & AF_TILE1) && (nc & 0x80)) {
            /* Flush */
            if (fn) {
                /* Draw pending chars (normal) */
                if (fa || always_text) {
                    (void)((*Term->text_hook)(fx, y, fn, fa, &scr_cc[fx]));
                }

                /* Draw pending chars (black) */
                else {
                    (void)((*Term->wipe_hook)(fx, y, fn));
                }

                /* Forget */
                fn = 0;
            }

            /* Draw the special attr/char pair */
            (void)((*Term->pict_hook)(x, y, 1, &na, &nc, &nta, &ntc));

            /* Skip */
            continue;
        }

        /* Notice new color */
#ifdef JP
        if (fa != (na & AF_KANJIC))
#else
        if (fa != na)
#endif

        {
            /* Flush */
            if (fn) {
                /* Draw the pending chars */
                if (fa || always_text) {
                    (void)((*Term->text_hook)(fx, y, fn, fa, &scr_cc[fx]));
                }

                /* Erase "leading" spaces */
                else {
                    (void)((*Term->wipe_hook)(fx, y, fn));
                }

                /* Forget */
                fn = 0;
            }

            /* Save the new color */
#ifdef JP
            fa = (na & AF_KANJIC);
#else
            fa = na;
#endif
        }

        /* Restart and Advance */
        if (fn++ == 0)
            fx = x;
    }

    /* Flush */
    if (fn) {
        /* Draw pending chars (normal) */
        if (fa || always_text) {
            (void)((*Term->text_hook)(fx, y, fn, fa, &scr_cc[fx]));
        }

        /* Draw pending chars (black) */
        else {
            (void)((*Term->wipe_hook)(fx, y, fn));
        }
    }
}

/*
 * Flush a row of the current window (see "term_fresh")
 *
 * Display text using "term_text()" and "term_wipe()"
 */
static void term_fresh_row_text(TERM_LEN y, TERM_LEN x1, TERM_LEN x2)
{
    auto &old_aa = Term->old->a[y];
    auto &old_cc = Term->old->c[y];

    const auto &scr_aa = Term->scr->a[y];
    const auto &scr_cc = Term->scr->c[y];

    /* The "always_text" flag */
    int always_text = Term->always_text;

    /* Pending length */
    int fn = 0;

    /* Pending start */
    int fx = 0;

    /* Pending attr */
    byte fa = Term->attr_blank;

    TERM_COLOR oa;
    char oc;

    TERM_COLOR na;
    char nc;

#ifdef JP
    /* 全角文字の２バイト目かどうか */
    int kanji = 0;

    for (TERM_LEN x = 0; x < x1; x++)
        if (!(old_aa[x] & AF_TILE1) && iskanji(old_cc[x])) {
            if (x == x1 - 1) {
                x1--;
                break;
            } else
                x++;
        }
#endif
    /* Scan "modified" columns */
    for (TERM_LEN x = x1; x <= x2; x++) {
        /* See what is currently here */
        oa = old_aa[x];
        oc = old_cc[x];

        /* See what is desired there */
        na = scr_aa[x];
        nc = scr_cc[x];

#ifdef JP
        if (kanji) {
            /* 全角文字２バイト目 */
            kanji = 0;
            old_aa[x] = na;
            old_cc[x] = nc;
            fn++;
            continue;
        }
        /* 特殊文字としてMSBが立っている可能性がある */
        /* その場合attrのMSBも立っているのでこれで識別する */
        /* check */
        kanji = (iskanji(nc) && !(na & AF_TILE1));
#endif
        /* Handle unchanged grids */
#ifdef JP
        if ((na == oa) && (nc == oc) && (!kanji || (scr_aa[x + 1] == old_aa[x + 1] && scr_cc[x + 1] == old_cc[x + 1])))
#else
        if ((na == oa) && (nc == oc))
#endif

        {
            /* Flush */
            if (fn) {
                /* Draw pending chars (normal) */
                if (fa || always_text) {
                    (void)((*Term->text_hook)(fx, y, fn, fa, &scr_cc[fx]));
                }

                /* Draw pending chars (black) */
                else {
                    (void)((*Term->wipe_hook)(fx, y, fn));
                }

                /* Forget */
                fn = 0;
            }

#ifdef JP
            /* 全角文字の時は再開位置は＋１ */
            if (kanji) {
                x++;
                fx++;
                kanji = 0;
            }
#endif
            /* Skip */
            continue;
        }

        /* Save new contents */
        old_aa[x] = na;
        old_cc[x] = nc;

        /* Notice new color */
#ifdef JP
        if (fa != (na & AF_KANJIC))
#else
        if (fa != na)
#endif

        {
            /* Flush */
            if (fn) {
                /* Draw the pending chars */
                if (fa || always_text) {
                    (void)((*Term->text_hook)(fx, y, fn, fa, &scr_cc[fx]));
                }

                /* Erase "leading" spaces */
                else {
                    (void)((*Term->wipe_hook)(fx, y, fn));
                }

                /* Forget */
                fn = 0;
            }

            /* Save the new color */
#ifdef JP
            fa = (na & AF_KANJIC);
#else
            fa = na;
#endif
        }

        /* Restart and Advance */
        if (fn++ == 0)
            fx = x;
    }

    /* Flush */
    if (fn) {
        /* Draw pending chars (normal) */
        if (fa || always_text) {
            (void)((*Term->text_hook)(fx, y, fn, fa, &scr_cc[fx]));
        }

        /* Draw pending chars (black) */
        else {
            (void)((*Term->wipe_hook)(fx, y, fn));
        }
    }
}

/*
 * @brief Actually perform all requested changes to the window
 */
errr term_fresh(void)
{
    int w = Term->wid;
    int h = Term->hgt;

    int y1 = Term->y1;
    int y2 = Term->y2;

    const auto &old = Term->old;
    const auto &scr = Term->scr;

    /* Before initialize (Advice from Mr.shimitei)*/
    if (!old || !scr)
        return 1;

    if (Term->never_fresh)
        return 1;

    /* Do nothing unless "mapped" */
    if (!Term->mapped_flag)
        return 1;

    /* Trivial Refresh */
    if ((y1 > y2) && (scr->cu == old->cu) && (scr->cv == old->cv) && (scr->cx == old->cx) && (scr->cy == old->cy) && !(Term->total_erase)) {
        /* Nothing */
        return 1;
    }

    /* Handle "total erase" */
    if (Term->total_erase) {
        byte na = Term->attr_blank;
        char nc = Term->char_blank;

        /* Physically erase the entire window */
        term_xtra(TERM_XTRA_CLEAR, 0);

        /* clear all "cursor" data */
        old->cv = old->cu = false;
        old->cx = old->cy = 0;

        /* Wipe each row */
        for (TERM_LEN y = 0; y < h; y++) {
            auto &aa = old->a[y];
            auto &cc = old->c[y];

            auto &taa = old->ta[y];
            auto &tcc = old->tc[y];

            /* Wipe each column */
            for (TERM_LEN x = 0; x < w; x++) {
                /* Wipe each grid */
                aa[x] = na;
                cc[x] = nc;

                taa[x] = na;
                tcc[x] = nc;
            }
        }

        /* Redraw every row */
        Term->y1 = y1 = 0;
        Term->y2 = y2 = h - 1;

        /* Redraw every column */
        for (TERM_LEN y = 0; y < h; y++) {
            Term->x1[y] = 0;
            Term->x2[y] = w - 1;
        }

        /* Forget "total erase" */
        Term->total_erase = false;
    }

    /* Cursor update -- Erase old Cursor */
    if (Term->soft_cursor) {
        /* Cursor was visible */
        if (!old->cu && old->cv) {
            int csize = 1;
            TERM_LEN tx = old->cx;
            TERM_LEN ty = old->cy;

            const auto &old_aa = old->a[ty];
            const auto &old_cc = old->c[ty];

            const auto &old_taa = old->ta[ty];
            const auto &old_tcc = old->tc[ty];

            TERM_COLOR ota = old_taa[tx];
            char otc = old_tcc[tx];

#ifdef JP
            if (tx + 1 < Term->wid && !(old_aa[tx] & AF_TILE1) && iskanji(old_cc[tx]))
                csize = 2;
#endif
            /* Use "term_pict()" always */
            if (Term->always_pict)
                (void)((*Term->pict_hook)(tx, ty, csize, &old_aa[tx], &old_cc[tx], &ota, &otc));

            /* Use "term_pict()" sometimes */
            else if (Term->higher_pict && (old_aa[tx] & AF_TILE1) && (old_cc[tx] & 0x80))
                (void)((*Term->pict_hook)(tx, ty, 1, &old_aa[tx], &old_cc[tx], &ota, &otc));

            /*
             * Restore the actual character
             * 元の文字の描画範囲がカーソルより小さいと、
             * 上書きされなかった部分がゴミとして残る。
             * wipe_hook でカーソルを消去して text_hook で書き直す。
             */
            else if (old_aa[tx] || Term->always_text) {
                (void)((*Term->wipe_hook)(tx, ty, 1));
                (void)((*Term->text_hook)(tx, ty, csize, (unsigned char)(old_aa[tx] & 0xf), &old_cc[tx]));
            }

            /* Erase the grid */
            else
                (void)((*Term->wipe_hook)(tx, ty, 1));
        }
    }

    /* Cursor Update -- Erase old Cursor */
    else {
        /* Cursor will be invisible */
        if (scr->cu || !scr->cv) {
            /* Make the cursor invisible */
            term_xtra(TERM_XTRA_SHAPE, 0);
        }
    }

    /* Something to update */
    if (y1 <= y2) {
        /* Handle "icky corner" */
        if (Term->icky_corner) {
            /* Avoid the corner */
            if (y2 >= h - 1) {
                /* Avoid the corner */
                if (Term->x2[h - 1] > w - 2) {
                    /* Avoid the corner */
                    Term->x2[h - 1] = w - 2;
                }
            }
        }

        /* Scan the "modified" rows */
        for (TERM_LEN y = y1; y <= y2; ++y) {
            TERM_LEN x1 = Term->x1[y];
            TERM_LEN x2 = Term->x2[y];

            /* Flush each "modified" row */
            if (x1 <= x2) {
                /* Always use "term_pict()" */
                if (Term->always_pict) {
                    /* Flush the row */
                    term_fresh_row_pict(y, x1, x2);
                }

                /* Sometimes use "term_pict()" */
                else if (Term->higher_pict) {
                    /* Flush the row */
                    term_fresh_row_both(y, x1, x2);
                }

                /* Never use "term_pict()" */
                else {
                    /* Flush the row */
                    term_fresh_row_text(y, x1, x2);
                }

                /* This row is all done */
                Term->x1[y] = w;
                Term->x2[y] = 0;

                /* Flush that row (if allowed) */
                if (!Term->never_frosh)
                    term_xtra(TERM_XTRA_FROSH, y);
            }
        }

        /* No rows are invalid */
        Term->y1 = h;
        Term->y2 = 0;
    }

    /* Cursor update -- Show new Cursor */
    if (Term->soft_cursor) {
        /* Draw the cursor */
        if (!scr->cu && scr->cv) {
#ifdef JP
            if ((scr->cx + 1 < w)
                && ((old->a[scr->cy][scr->cx + 1] & AF_BIGTILE2) == AF_BIGTILE2
                    || (!(old->a[scr->cy][scr->cx] & AF_TILE1) && iskanji(old->c[scr->cy][scr->cx]))))
#else
            if ((scr->cx + 1 < w) && (old->a[scr->cy][scr->cx + 1] & AF_BIGTILE2) == AF_BIGTILE2)
#endif
            {
                /* Double width cursor for the Bigtile mode */
                (void)((*Term->bigcurs_hook)(scr->cx, scr->cy));
            } else {
                /* Call the cursor display routine */
                (void)((*Term->curs_hook)(scr->cx, scr->cy));
            }
        }
    }

    /* Cursor Update -- Show new Cursor */
    else {
        /* The cursor is useless, hide it */
        if (scr->cu) {
            /* Paranoia -- Put the cursor NEAR where it belongs */
            (void)((*Term->curs_hook)(w - 1, scr->cy));

            /* Make the cursor invisible */
            /* term_xtra(TERM_XTRA_SHAPE, 0); */
        }

        /* The cursor is invisible, hide it */
        else if (!scr->cv) {
            /* Paranoia -- Put the cursor where it belongs */
            (void)((*Term->curs_hook)(scr->cx, scr->cy));

            /* Make the cursor invisible */
            /* term_xtra(TERM_XTRA_SHAPE, 0); */
        }

        /* The cursor is visible, display it correctly */
        else {
            /* Put the cursor where it belongs */
            (void)((*Term->curs_hook)(scr->cx, scr->cy));

            /* Make the cursor visible */
            term_xtra(TERM_XTRA_SHAPE, 1);
        }
    }

    /* Save the "cursor state" */
    old->cu = scr->cu;
    old->cv = scr->cv;
    old->cx = scr->cx;
    old->cy = scr->cy;

    /* Actually flush the output */
    term_xtra(TERM_XTRA_FRESH, 0);
    return 0;
}

/*
 * @brief never_freshの値を無視して強制的にterm_freshを行う。
 */
errr term_fresh_force(void)
{
    bool old = Term->never_fresh;
    Term->never_fresh = false;
    errr err = term_fresh();
    Term->never_fresh = old;
    return err;
}

/*** Output routines ***/

/*
 * Set the cursor visibility
 */
errr term_set_cursor(int v)
{
    /* Already done */
    if (Term->scr->cv == (bool)v)
        return 1;

    /* Change */
    Term->scr->cv = (bool)v;
    return 0;
}

/*
 * Place the cursor at a given location
 *
 * Note -- "illegal" requests do not move the cursor.
 */
errr term_gotoxy(TERM_LEN x, TERM_LEN y)
{
    int w = Term->wid;
    int h = Term->hgt;

    /* Verify */
    if ((x < 0) || (x >= w))
        return -1;
    if ((y < 0) || (y >= h))
        return -1;

    /* Remember the cursor */
    Term->scr->cx = x;
    Term->scr->cy = y;

    /* The cursor is not useless */
    Term->scr->cu = 0;
    return 0;
}

/*
 * At a given location, place an attr/char
 * Do not change the cursor position
 * No visual changes until "term_fresh()".
 */
errr term_draw(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c)
{
    int w = Term->wid;
    int h = Term->hgt;

    if ((x < 0) || (x >= w))
        return -1;
    if ((y < 0) || (y >= h))
        return -1;

    /* Paranoia -- illegal char */
    if (!c)
        return (-2);

    /* Queue it for later */
    term_queue_char(x, y, a, c, 0, 0);
    return 0;
}

/*
 * Using the given attr, add the given char at the cursor.
 *
 * We return "-2" if the character is "illegal". XXX XXX
 *
 * We return "-1" if the cursor is currently unusable.
 *
 * We queue the given attr/char for display at the current
 * cursor location, and advance the cursor to the right,
 * marking it as unuable and returning "1" if it leaves
 * the screen, and otherwise returning "0".
 *
 * So when this function, or the following one, return a
 * positive value, future calls to either function will
 * return negative ones.
 */
errr term_addch(TERM_COLOR a, char c)
{
    TERM_LEN w = Term->wid;

    /* Handle "unusable" cursor */
    if (Term->scr->cu)
        return -1;

    /* Paranoia -- no illegal chars */
    if (!c)
        return (-2);

    /* Queue the given character for display */
    term_queue_char(Term->scr->cx, Term->scr->cy, a, c, 0, 0);

    /* Advance the cursor */
    Term->scr->cx++;

    /* Success */
    if (Term->scr->cx < w)
        return 0;

    /* Note "Useless" cursor */
    Term->scr->cu = 1;

    /* Note "Useless" cursor */
    return 1;
}

/*
 * Bigtile version of term_addch().
 *
 * If use_bigtile is FALSE, simply call term_addch() .
 *
 * Otherwise, queue a pair of attr/char for display at the current
 * cursor location, and advance the cursor to the right by two.
 */
errr term_add_bigch(TERM_COLOR a, char c)
{
    if (!use_bigtile)
        return term_addch(a, c);

    /* Handle "unusable" cursor */
    if (Term->scr->cu)
        return -1;

    /* Paranoia -- no illegal chars */
    if (!c)
        return (-2);

    /* Queue the given character for display */
    term_queue_bigchar(Term->scr->cx, Term->scr->cy, a, c, 0, 0);

    /* Advance the cursor */
    Term->scr->cx += 2;

    /* Success */
    if (Term->scr->cx < Term->wid)
        return 0;

    /* Note "Useless" cursor */
    Term->scr->cu = 1;

    /* Note "Useless" cursor */
    return 1;
}

/*
 * At the current location, using an attr, add a string
 *
 * We also take a length "n", using negative values to imply
 * the largest possible value, and then we use the minimum of
 * this length and the "actual" length of the string as the
 * actual number of characters to attempt to display, never
 * displaying more characters than will actually fit, since
 * we do NOT attempt to "wrap" the cursor at the screen edge.
 *
 * We return "-1" if the cursor is currently unusable.
 * We return "N" if we were "only" able to write "N" chars,
 * even if all of the given characters fit on the screen,
 * and mark the cursor as unusable for future attempts.
 *
 * So when this function, or the preceding one, return a
 * positive value, future calls to either function will
 * return negative ones.
 */
errr term_addstr(int n, TERM_COLOR a, concptr s)
{
    int k;
    TERM_LEN w = Term->wid;
    errr res = 0;

    /* Handle "unusable" cursor */
    if (Term->scr->cu)
        return -1;

    /* Obtain maximal length */
    k = (n < 0) ? (w + 1) : n;

    /* Obtain the usable string length */
    for (n = 0; (n < k) && s[n]; n++) /* loop */
        ;

    /* React to reaching the edge of the screen */
    if (Term->scr->cx + n >= w)
        res = n = w - Term->scr->cx;

    /* Queue the first "n" characters for display */
    term_queue_chars(Term->scr->cx, Term->scr->cy, n, a, s);

    /* Advance the cursor */
    Term->scr->cx += n;

    /* Notice "Useless" cursor */
    if (res)
        Term->scr->cu = 1;

    return res;
}

/*
 * Move to a location and, using an attr, add a char
 */
errr term_putch(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c)
{
    errr res;

    /* Move first */
    if ((res = term_gotoxy(x, y)) != 0)
        return (res);

    /* Then add the char */
    if ((res = term_addch(a, c)) != 0)
        return (res);

    return 0;
}

/*
 * Move to a location and, using an attr, add a string
 */
errr term_putstr(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr s)
{
    errr res;

    /* Move first */
    if ((res = term_gotoxy(x, y)) != 0)
        return (res);

    /* Then add the string */
    if ((res = term_addstr(n, a, s)) != 0)
        return (res);

    return 0;
}

/*
 * Place cursor at (x,y), and clear the next "n" chars
 */
errr term_erase(TERM_LEN x, TERM_LEN y, int n)
{
    TERM_LEN w = Term->wid;
    /* int h = Term->hgt; */

    TERM_LEN x1 = -1;
    TERM_LEN x2 = -1;

    int na = Term->attr_blank;
    int nc = Term->char_blank;

    /* Place cursor */
    if (term_gotoxy(x, y))
        return -1;

    /* Force legal size */
    if (x + n > w)
        n = w - x;

    /* Fast access */
    auto &scr_aa = Term->scr->a[y];
    auto &scr_cc = Term->scr->c[y];

    auto &scr_taa = Term->scr->ta[y];
    auto &scr_tcc = Term->scr->tc[y];

#ifdef JP
    /*
     * 全角文字の右半分から文字を表示する場合、
     * 重なった文字の左部分を消去。
     */
    if (n > 0 && (((scr_aa[x] & AF_KANJI2) && !(scr_aa[x] & AF_TILE1)) || (scr_aa[x] & AF_BIGTILE2) == AF_BIGTILE2))
#else
    if (n > 0 && (scr_aa[x] & AF_BIGTILE2) == AF_BIGTILE2)
#endif
    {
        x--;
        n++;
    }

    /* Scan every column */
    for (int i = 0; i < n; i++, x++) {
        int oa = scr_aa[x];
        int oc = scr_cc[x];

        /* Ignore "non-changes" */
        if ((oa == na) && (oc == nc))
            continue;

#ifdef JP
        /*
         * 全角文字の左半分で表示を終了する場合、
         * 重なった文字の右部分を消去。
         *
         * 2001/04/29 -- Habu
         * 行の右端の場合はこの処理をしないように修正。
         */
        if ((oa & AF_KANJI1) && (i + 1) == n && x != w - 1)
            n++;
#endif
        /* Save the "literal" information */
        scr_aa[x] = (byte)na;
        scr_cc[x] = (char)nc;

        scr_taa[x] = 0;
        scr_tcc[x] = 0;

        /* Track minimum changed column */
        if (x1 < 0)
            x1 = x;

        /* Track maximum changed column */
        x2 = x;
    }

    /* Expand the "change area" as needed */
    if (x1 >= 0) {
        /* Check for new min/max row info */
        if (y < Term->y1)
            Term->y1 = y;
        if (y > Term->y2)
            Term->y2 = y;

        /* Check for new min/max col info in this row */
        if (x1 < Term->x1[y])
            Term->x1[y] = x1;
        if (x2 > Term->x2[y])
            Term->x2[y] = x2;
    }

    return 0;
}

/*
 * Clear the entire window, and move to the top left corner
 *
 * Note the use of the special "total_erase" code
 */
errr term_clear(void)
{
    TERM_LEN w = Term->wid;
    TERM_LEN h = Term->hgt;

    TERM_COLOR na = Term->attr_blank;
    char nc = Term->char_blank;

    /* Cursor usable */
    Term->scr->cu = 0;

    /* Cursor to the top left */
    Term->scr->cx = Term->scr->cy = 0;

    /* Wipe each row */
    for (TERM_LEN y = 0; y < h; y++) {
        auto &scr_aa = Term->scr->a[y];
        auto &scr_cc = Term->scr->c[y];

        auto &scr_taa = Term->scr->ta[y];
        auto &scr_tcc = Term->scr->tc[y];

        /* Wipe each column */
        for (TERM_LEN x = 0; x < w; x++) {
            scr_aa[x] = na;
            scr_cc[x] = nc;

            scr_taa[x] = 0;
            scr_tcc[x] = 0;
        }

        /* This row has changed */
        Term->x1[y] = 0;
        Term->x2[y] = w - 1;
    }

    /* Every row has changed */
    Term->y1 = 0;
    Term->y2 = h - 1;

    /* Force "total erase" */
    Term->total_erase = true;
    return 0;
}

/*
 * Redraw (and refresh) the whole window.
 */
errr term_redraw(void)
{
    /* Force "total erase" */
    Term->total_erase = true;
    term_fresh();
    return 0;
}

/*
 * Redraw part of a window.
 */
errr term_redraw_section(TERM_LEN x1, TERM_LEN y1, TERM_LEN x2, TERM_LEN y2)
{
    /* Bounds checking */
    if (y2 >= Term->hgt)
        y2 = Term->hgt - 1;
    if (x2 >= Term->wid)
        x2 = Term->wid - 1;
    if (y1 < 0)
        y1 = 0;
    if (x1 < 0)
        x1 = 0;

    /* Set y limits */
    Term->y1 = y1;
    Term->y2 = y2;

    /* Set the x limits */
    for (int i = Term->y1; i <= Term->y2; i++) {
#ifdef JP
        TERM_LEN x1j = x1;
        TERM_LEN x2j = x2;

        if (x1j > 0) {
            if (Term->scr->a[i][x1j] & AF_KANJI2)
                x1j--;
        }

        if (x2j < Term->wid - 1) {
            if (Term->scr->a[i][x2j] & AF_KANJI1)
                x2j++;
        }

        Term->x1[i] = x1j;
        Term->x2[i] = x2j;

        auto &g_ptr = Term->old->c[i];

        /* Clear the section so it is redrawn */
        for (int j = x1j; j <= x2j; j++) {
            /* Hack - set the old character to "none" */
            g_ptr[j] = 0;
        }
#else
        Term->x1[i] = x1;
        Term->x2[i] = x2;

        auto &g_ptr = Term->old->c[i];

        /* Clear the section so it is redrawn */
        for (int j = x1; j <= x2; j++) {
            /* Hack - set the old character to "none" */
            g_ptr[j] = 0;
        }
#endif
    }

    term_fresh();
    return 0;
}

/*** Access routines ***/

/*
 * Extract the cursor visibility
 */
errr term_get_cursor(int *v)
{
    /* Extract visibility */
    (*v) = Term->scr->cv;
    return 0;
}

/*
 * Extract the current window size
 */
errr term_get_size(TERM_LEN *w, TERM_LEN *h)
{
    /* Access the cursor */
    (*w) = Term->wid;
    (*h) = Term->hgt;
    return 0;
}

/*
 * Extract the current cursor location
 */
errr term_locate(TERM_LEN *x, TERM_LEN *y)
{
    /* Access the cursor */
    (*x) = Term->scr->cx;
    (*y) = Term->scr->cy;

    /* Warn about "useless" cursor */
    if (Term->scr->cu)
        return 1;

    return 0;
}

/*
 * At a given location, determine the "current" attr and char
 * Note that this refers to what will be on the window after the
 * next call to "term_fresh()".  It may or may not already be there.
 */
errr term_what(TERM_LEN x, TERM_LEN y, TERM_COLOR *a, char *c)
{
    TERM_LEN w = Term->wid;
    TERM_LEN h = Term->hgt;

    if ((x < 0) || (x >= w))
        return -1;
    if ((y < 0) || (y >= h))
        return -1;

    /* Direct access */
    (*a) = Term->scr->a[y][x];
    (*c) = Term->scr->c[y][x];
    return 0;
}

/*** Input routines ***/

/*
 * Flush and forget the input
 */
errr term_flush(void)
{
    /* Flush all events */
    term_xtra(TERM_XTRA_FLUSH, 0);

    /* Forget all keypresses */
    Term->key_head = Term->key_tail = 0;
    return 0;
}

/*
 * Add a keypress to the FRONT of the "queue"
 */
errr term_key_push(int k)
{
    /* Refuse to enqueue non-keys */
    if (!k)
        return -1;

    /* Overflow may induce circular queue */
    if (Term->key_tail == 0)
        Term->key_tail = Term->key_size;

    /* Back up, Store the char */
    Term->key_queue[--Term->key_tail] = (char)k;

    if (Term->key_head != Term->key_tail)
        return 0;

    return 1;
}

/*
 * Check for a pending keypress on the key queue.
 *
 * Store the keypress, if any, in "ch", and return "0".
 * Otherwise store "zero" in "ch", and return "1".
 *
 * Wait for a keypress if "wait" is true.
 *
 * Remove the keypress if "take" is true.
 */
errr term_inkey(char *ch, bool wait, bool take)
{
    /* Assume no key */
    (*ch) = '\0';

    /* get bored */
    if (!Term->never_bored) {
        /* Process random events */
        term_xtra(TERM_XTRA_BORED, 0);
    }

    /* Wait */
    if (wait) {
        /* Process pending events while necessary */
        while (Term->key_head == Term->key_tail) {
            /* Process events (wait for one) */
            term_xtra(TERM_XTRA_EVENT, true);
        }
    }

    /* Do not Wait */
    else {
        /* Process pending events if necessary */
        if (Term->key_head == Term->key_tail) {
            /* Process events (do not wait) */
            term_xtra(TERM_XTRA_EVENT, false);
        }
    }

    /* No keys are ready */
    if (Term->key_head == Term->key_tail)
        return 1;

    /* Extract the next keypress */
    (*ch) = Term->key_queue[Term->key_tail];

    /* If requested, advance the queue, wrap around if necessary */
    if (take && (++Term->key_tail == Term->key_size))
        Term->key_tail = 0;

    return 0;
}

/*** Extra routines ***/

/*
 * Save the "requested" screen into the "memorized" screen
 *
 * Every "term_save()" should match exactly one "term_load()"
 */
errr term_save(void)
{
    /* Push stack */
    Term->mem_stack.push(Term->scr->clone());

    return 0;
}

/*
 * Restore the "requested" contents (see above).
 *
 * Every "term_save()" should match exactly one "term_load()"
 */
errr term_load(bool load_all)
{
    TERM_LEN w = Term->wid;
    TERM_LEN h = Term->hgt;

    if (Term->mem_stack.empty())
        return 0;

    if (load_all) {
        // 残り1つを残して読み捨てる
        while (Term->mem_stack.size() > 1)
            Term->mem_stack.pop();
    }

    /* Load */
    Term->scr.swap(Term->mem_stack.top());
    Term->scr->resize(w, h);

    /* Pop stack */
    Term->mem_stack.pop();

    /* Assume change */
    for (TERM_LEN y = 0; y < h; y++) {
        /* Assume change */
        Term->x1[y] = 0;
        Term->x2[y] = w - 1;
    }

    /* Assume change */
    Term->y1 = 0;
    Term->y2 = h - 1;
    return 0;
}

/*
 * Exchange the "requested" screen with the "tmp" screen
 */
errr term_exchange(void)
{
    TERM_LEN w = Term->wid;
    TERM_LEN h = Term->hgt;

    /* Create */
    if (!Term->tmp) {
        /* Allocate window */
        Term->tmp = term_win::create(w, h);
    }

    /* Swap */
    Term->scr.swap(Term->tmp);

    /* Assume change */
    for (TERM_LEN y = 0; y < h; y++) {
        /* Assume change */
        Term->x1[y] = 0;
        Term->x2[y] = w - 1;
    }

    /* Assume change */
    Term->y1 = 0;
    Term->y2 = h - 1;
    return 0;
}

/*
 * React to a new physical window size.
 */
errr term_resize(TERM_LEN w, TERM_LEN h)
{
    /* Resizing is forbidden */
    if (Term->fixed_shape)
        return -1;

    /* Ignore illegal changes */
    if ((w < 1) || (h < 1))
        return -1;

    /* Ignore non-changes */
    if ((Term->wid == w) && (Term->hgt == h) && (arg_bigtile == use_bigtile))
        return 1;

    use_bigtile = arg_bigtile;

    /* Resize windows */
    Term->old->resize(w, h);
    Term->scr->resize(w, h);
    if (Term->tmp)
        Term->tmp->resize(w, h);

    /* Resize scanners */
    Term->x1.resize(h);
    Term->x2.resize(h);

    /* Save new size */
    Term->wid = w;
    Term->hgt = h;

    /* Force "total erase" */
    Term->total_erase = true;

    /* Assume change */
    for (int i = 0; i < h; i++) {
        /* Assume change */
        Term->x1[i] = 0;
        Term->x2[i] = w - 1;
    }

    /* Assume change */
    Term->y1 = 0;
    Term->y2 = h - 1;

    /* Execute the "resize_hook" hook, if available */
    if (Term->resize_hook)
        Term->resize_hook();

    return 0;
}

/*
 * Activate a new Term (and deactivate the current Term)
 *
 * This function is extremely important, and also somewhat bizarre.
 * It is the only function that should "modify" the value of "Term".
 *
 * To "create" a valid "term", one should do "term_init(t)", then
 * set the various flags and hooks, and then do "term_activate(t)".
 */
errr term_activate(term_type *t)
{
    /* already done */
    if (Term == t)
        return 1;

    /* Deactivate the old Term */
    if (Term)
        term_xtra(TERM_XTRA_LEVEL, 0);

    /* Call the special "init" hook */
    if (t && !t->active_flag) {
        /* Call the "init" hook */
        if (t->init_hook)
            (*t->init_hook)(t);

        /* Remember */
        t->active_flag = true;

        /* Assume mapped */
        t->mapped_flag = true;
    }

    /* Remember the Term */
    Term = t;

    /* Activate the new Term */
    if (Term)
        term_xtra(TERM_XTRA_LEVEL, 1);

    return 0;
}

/*
 * Initialize a term, using a window of the given size.
 * Also prepare the "input queue" for "k" keypresses
 * By default, the cursor starts out "invisible"
 * By default, we "erase" using "black spaces"
 */
errr term_init(term_type *t, TERM_LEN w, TERM_LEN h, int k)
{
    /* Wipe it */
    *t = term_type{};

    /* Prepare the input queue */
    t->key_head = t->key_tail = 0;

    /* Determine the input queue size */
    t->key_size = (uint16_t)k;

    /* Allocate the input queue */
    t->key_queue.resize(t->key_size);

    /* Save the size */
    t->wid = w;
    t->hgt = h;

    /* Allocate change arrays */
    t->x1.resize(h);
    t->x2.resize(h);

    /* Allocate "displayed" */
    t->old = term_win::create(w, h);

    /* Allocate "requested" */
    t->scr = term_win::create(w, h);

    /* Assume change */
    for (TERM_LEN y = 0; y < h; y++) {
        /* Assume change */
        t->x1[y] = 0;
        t->x2[y] = w - 1;
    }

    /* Assume change */
    t->y1 = 0;
    t->y2 = h - 1;

    /* Force "total erase" */
    t->total_erase = true;

    /* Default "blank" */
    t->attr_blank = 0;
    t->char_blank = ' ';

    /* Prepare "fake" hooks to prevent core dumps */
    t->curs_hook = term_curs_hack;
    t->bigcurs_hook = term_bigcurs_hack;
    t->wipe_hook = term_wipe_hack;
    t->text_hook = term_text_hack;
    t->pict_hook = term_pict_hack;
    return 0;
}

#ifdef JP
/*
 * Move to a location and, using an attr, add a string vertically
 */
errr term_putstr_v(TERM_LEN x, TERM_LEN y, int n, byte a, concptr s)
{
    errr res;
    int y0 = y;

    for (int i = 0; i < n && s[i] != 0; i++) {
        /* Move first */
        if ((res = term_gotoxy(x, y0)) != 0)
            return (res);

        if (iskanji(s[i])) {
            if ((res = term_addstr(2, a, &s[i])) != 0)
                return (res);
            i++;
            y0++;
            if (s[i] == 0)
                break;
        } else {
            if ((res = term_addstr(1, a, &s[i])) != 0)
                return (res);
            y0++;
        }
    }

    return 0;
}
#endif

#ifndef WINDOWS
errr term_nuke(term_type *t)
{
    if (t->active_flag) {
        if (t->nuke_hook)
            (*t->nuke_hook)(t);

        t->active_flag = false;
        t->mapped_flag = false;
    }

    t->old.reset();
    t->scr.reset();
    t->tmp.reset();
    while (!t->mem_stack.empty())
        t->mem_stack.pop();

    t->x1.clear();
    t->x2.clear();
    t->key_queue.clear();
    return 0;
}
#endif
