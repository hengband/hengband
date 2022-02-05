﻿/* File: main-gcu.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/* Purpose: Allow use of Unix "curses" with Angband -BEN- */

/*
 * This file has been modified to use multiple text windows if your screen
 * is larger than 80x25.  By Keldon Jones (keldon@umr.edu).
 *
 * Also included is Keldon Jones patch to get better colors. To switch to
 * a term that supports this, see this posting:
 *
 * From keldon@umr.edu Thu Apr 01 05:40:14 1999
 * Sender: KELDON JONES <keldon@saucer.cc.umr.edu>
 * From: Keldon Jones <keldon@umr.edu>
 * Subject: Re: Linux colour prob (Or: question for Greg)
 * Newsgroups: rec.games.roguelike.angband
 * References: <slrn7g1jlp.gj9.scarblac-spamtrap@flits104-37.flits.rug.nl> <3700f96b.1593384@news.polsl.gliwice.pl> <slrn7g36er.fm4.wooledge@jekyll.local>
 * X-Newsreader: TIN [UNIX 1.3 unoff BETA 970625; 9000/780 HP-UX B.10.20]
 * NNTP-Posting-Host: saucer.cc.umr.edu
 * X-NNTP-Posting-Host: saucer.cc.umr.edu
 * Message-ID: <370306be.0@news.cc.umr.edu>
 * Date: 1 Apr 99 05:40:14 GMT
 * Organization: University of Missouri - Rolla
 * Lines: 199
 * Path:
 * xs4all!xs4all!newsfeed.wirehub.nl!news-peer.gip.net!news.gsl.net!gip.net!news.he.net!mercury.cts.com!alpha.sky.net!news.missouri.edu!news.cc.umr.edu!not-for-mail
 * Xref: xs4all rec.games.roguelike.angband:86332
 *
 * Greg Wooledge <wooledge@kellnet.com> wrote:
 * > Gwidon S. Naskrent (naskrent@artemida.amu.edu.pl) wrote:
 *
 * > >On 30 Mar 1999 13:17:18 GMT, scarblac-spamtrap@pino.selwerd.cx (Remco
 * > >Gerlich) wrote:
 *
 * > >>I recently switched to Linux, and *bands work fine. I like
 * > >>to play them in consoles, not in X. However, colour is wrong.
 * > >>"Slate" and "light slate" are always light blue, instead
 * > >>of some shade of grey. Colours are fine in X.
 *
 * > I actually noticed the Linux console color issue a very long time ago,
 * > but since I always play under X, I never really investigated it.
 *
 * > You're absolutely right, though -- the Linux console colors are not
 * > "right" for Angband.
 *
 *    I've noticed this myself, so I spent the evening fixing it.
 * Well, sorta fixing it.  It's not perfect yet, and it may not be
 * possible to get it perfect with VGA hardware and/or the current
 * Linux kernel.
 *
 * > OK, reading on in terminfo(5):
 *
 * >    Color Handling
 * >        Most color terminals are either `Tektronix-like'  or  `HP-
 * >        like'.   Tektronix-like terminals have a predefined set of
 * >        N colors (where N usually 8), and can  set  character-cell
 * >        foreground and background characters independently, mixing
 * >        them into N * N color-pairs.  On  HP-like  terminals,  the
 * >        use must set each color pair up separately (foreground and
 * >        background are  not  independently  settable).   Up  to  M
 * >        color-pairs  may  be  set  up  from  2*M different colors.
 * >        ANSI-compatible terminals are Tektronix-like.
 *
 * > The "linux" terminfo entry is definitely in the "Tektronix-like" family.
 * > It has the "setaf" and "setab" capabilities for setting the foreground
 * > and background colors to one of 8 basically hard-coded values:
 *
 * >              Color       #define       Value       RGB
 * >              black     COLOR_BLACK       0     0, 0, 0
 * >              red       COLOR_RED         1     max,0,0
 * >              green     COLOR_GREEN       2     0,max,0
 * >              yellow    COLOR_YELLOW      3     max,max,0
 * >              blue      COLOR_BLUE        4     0,0,max
 * >              magenta   COLOR_MAGENTA     5     max,0,max
 * >              cyan      COLOR_CYAN        6     0,max,max
 * >              white     COLOR_WHITE       7     max,max,max
 *
 *    Well, not quite.  Using certain escape sequences, an
 * application (or better yet, curses) can redefine the colors (at
 * least some of them) and then those are used.  Read the
 * curs_color manpage, and the part about "ccc" and "initc" in the
 * terminfo manpage.  This is what the part of main-gcu inside the
 * "if (can_fix_color)" code does.
 *
 * > So, what does this mean to the Angband player?  Well, it means that
 * > either there's nothing you can do about the console colors as long as
 * > straight curses/ncurses is used, or if there is something to be done,
 * > I'm not clever enough to figure out how to do it.
 *
 *    Well, it is possible, though you have to patch main-gcu
 * and edit a terminfo entry.  Apparently the relevant code in
 * main-gcu was never tested (it's broken in at least one major
 * way).  Apply the patch at the end of this message (notice that
 * we need to define REDEFINE_COLORS at some point near the
 * beginning of the file).
 *    Next, write this termcap entry to a file:
 *
 * linux-c|linux console 1.3.6+ with private palette for each virtual console,
 *         ccc,
 *         colors#16, pairs#64,
 *         initc=\E]P%x%p1%{16}%/%02x%p1%{16}%/%02x%p1%{16}%/%02x,
 *         oc=\E]R,
 *         use=linux,
 *
 * and run "tic" on it to produce a new terminfo entry called
 * "linux-c".  Especially note the "ccc" flag which says that we
 * can redefine colors.  The ugly "initc" string is what tells
 * the console how to redefine a color.  Now, just set your TERM
 * variable to "linux-c" and try Angband again.  If I've
 * remembered to tell you everything that I've done, you should
 * get the weird light-blue slate changed to a gray.
 *    Now, there are still lots of problems with this.
 * Something (I don't think it's curses, either the kernel or
 * the hardware itself) seems to be ignoring my color changes to
 * colors 6 and 7, which is annoying.  Also, the normal "white"
 * color is now way too bright, but it's now necessary to
 * distinguish it from the other grays.
 *    The kernel seems to support 16 colors, but you can
 * only switch to 8 of those, due to VT102 compatibility, it
 * seems.  I think it would be possible to patch the kernel and
 * allow all 16 colors to be used, but I haven't built up the
 * nerve to try that yet.
 *    Let me know if you can improve on this any.  Some of
 * this may actually work differently on other hardware (ugh).
 *
 *    Keldon
 *
 */

/*
 * To use this file, you must define "USE_GCU" in the Makefile.
 *
 * Hack -- note that "angband.h" is included AFTER the #ifdef test.
 * This was necessary because of annoying "curses.h" silliness.
 *
 * Note that this file is not "intended" to support non-Unix machines,
 * nor is it intended to support VMS or other bizarre setups.
 *
 * Also, this package assumes that the underlying "curses" handles both
 * the "nonl()" and "cbreak()" commands correctly, see the "OPTION" below.
 *
 * This code should work with most versions of "curses" or "ncurses",
 * and the "main-ncu.c" file (and USE_NCU define) are no longer used.
 *
 * See also "USE_CAP" and "main-cap.c" for code that bypasses "curses"
 * and uses the "termcap" information directly, or even bypasses the
 * "termcap" information and sends direct vt100 escape sequences.
 *
 * XXX XXX XXX Consider the use of "savetty()" and "resetty()".
 */

#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "io/exit-panic.h"
#include "io/files-util.h"
#include "locale/japanese.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "system/angband.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "view/display-map.h"

#ifdef USE_GCU

/*
 * Hack -- play games with "bool"
 */
#if __STDC_VERSION__ < 199901L
#undef bool
#endif

/*
 * Include the proper "header" file
 */
#include <curses.h>

/**
 * Simple rectangle type
 */
struct rect_s {
    int x, y;
    int cx, cy;
};
typedef struct rect_s rect_t, *rect_ptr;

/* Trivial rectangle utility to make code a bit more readable */
static rect_t rect(int x, int y, int cx, int cy)
{
    rect_t r;
    r.x = x;
    r.y = y;
    r.cx = cx;
    r.cy = cy;
    return r;
}

/**
 * Information about a term
 */
struct term_data {
    term_type t;
    rect_t r;
    WINDOW *win;
};

/* Max number of windows on screen */
#define MAX_TERM_DATA 4

/* Minimum main term size */
#define MIN_TERM0_LINES 24
#define MIN_TERM0_COLS 80

/* Information about our windows */
static term_data data[MAX_TERM_DATA];

/*
 * Hack -- try to guess which systems use what commands
 * Hack -- allow one of the "USE_Txxxxx" flags to be pre-set.
 * Mega-Hack -- try to guess when "POSIX" is available.
 * If the user defines two of these, we will probably crash.
 */
#if !defined(USE_TCHARS)
#if defined(_POSIX_VERSION)
#define USE_TPOSIX
#else
#define USE_TCHARS
#endif
#endif

/*
 * Try redefining the colors at startup.
 */
#define REDEFINE_COLORS

/*
 * POSIX stuff
 */
#ifdef USE_TPOSIX
#include <sys/ioctl.h>
#include <termios.h>
#endif

/*
 * One version needs this file
 */
#ifdef USE_TERMIO
#include <sys/ioctl.h>
#include <termio.h>
#endif

/*
 * The other needs this file
 */
#ifdef USE_TCHARS
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/types.h>
#endif

#include <locale.h>

/*
 * XXX XXX Hack -- POSIX uses "O_NONBLOCK" instead of "O_NDELAY"
 *
 * They should both work due to the "(i != 1)" test below.
 */
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

/*
 * OPTION: some machines lack "cbreak()"
 * On these machines, we use an older definition
 */
/* #define cbreak() crmode() */

/*
 * OPTION: some machines cannot handle "nonl()" and "nl()"
 * On these machines, we can simply ignore those commands.
 */
/* #define nonl() */
/* #define nl() */

static concptr ANGBAND_DIR_XTRA_SOUND;

/*
 * todo 有効活用されていない疑惑
 * Flag set once "sound" has been initialized
 */
static bool can_use_sound = false;

/*
 * An array of sound file names
 */
static concptr sound_file[SOUND_MAX];

/*
 * Save the "normal" and "angband" terminal settings
 */

#ifdef USE_TPOSIX

static struct termios norm_termios;

static struct termios game_termios;

#endif

#ifdef USE_TERMIO

static struct termio norm_termio;

static struct termio game_termio;

#endif

#ifdef USE_TCHARS
static struct ltchars norm_speciax_chars;
static struct sgttyb norm_ttyb;
static struct tchars norm_tchars;
static int norm_locax_chars;

static struct ltchars game_speciax_chars;
static struct sgttyb game_ttyb;
static struct tchars game_tchars;
static int game_locax_chars;
#endif

/*
 * Hack -- Number of initialized "term" structures
 */
static int active = 0;

#ifdef A_COLOR
/*
 * Hack -- define "A_BRIGHT" to be "A_BOLD", because on many
 * machines, "A_BRIGHT" produces ugly "inverse" video.
 */
#ifndef A_BRIGHT
#define A_BRIGHT A_BOLD
#endif

/*
 * Software flag -- we are allowed to use color
 */
static int can_use_color = false;

/*
 * Software flag -- we are allowed to change the colors
 */
static int can_fix_color = false;

/*
 * Simple Angband to Curses color conversion table
 */
static int colortable[16];

/**
 * Background color we should draw with; either BLACK or DEFAULT
 */
static int bg_color = COLOR_BLACK;

#endif

/*
 * Place the "keymap" into its "normal" state
 */
static void keymap_norm(void)
{
#ifdef USE_TPOSIX
    /* restore the saved values of the special chars */
    (void)tcsetattr(0, TCSAFLUSH, &norm_termios);
#endif

#ifdef USE_TERMIO
    /* restore the saved values of the special chars */
    (void)ioctl(0, TCSETA, (char *)&norm_termio);
#endif

#ifdef USE_TCHARS
    /* restore the saved values of the special chars */
    (void)ioctl(0, TIOCSLTC, (char *)&norm_speciax_chars);
    (void)ioctl(0, TIOCSETP, (char *)&norm_ttyb);
    (void)ioctl(0, TIOCSETC, (char *)&norm_tchars);
    (void)ioctl(0, TIOCLSET, (char *)&norm_locax_chars);
#endif
}

/*
 * Place the "keymap" into the "game" state
 */
static void keymap_game(void)
{
#ifdef USE_TPOSIX
    /* restore the saved values of the special chars */
    (void)tcsetattr(0, TCSAFLUSH, &game_termios);
#endif

#ifdef USE_TERMIO
    /* restore the saved values of the special chars */
    (void)ioctl(0, TCSETA, (char *)&game_termio);
#endif

#ifdef USE_TCHARS
    /* restore the saved values of the special chars */
    (void)ioctl(0, TIOCSLTC, (char *)&game_speciax_chars);
    (void)ioctl(0, TIOCSETP, (char *)&game_ttyb);
    (void)ioctl(0, TIOCSETC, (char *)&game_tchars);
    (void)ioctl(0, TIOCLSET, (char *)&game_locax_chars);
#endif
}

/*
 * Save the normal keymap
 */
static void keymap_norm_prepare(void)
{
#ifdef USE_TPOSIX
    /* Get the normal keymap */
    tcgetattr(0, &norm_termios);
#endif

#ifdef USE_TERMIO
    /* Get the normal keymap */
    (void)ioctl(0, TCGETA, (char *)&norm_termio);
#endif

#ifdef USE_TCHARS
    /* Get the normal keymap */
    (void)ioctl(0, TIOCGETP, (char *)&norm_ttyb);
    (void)ioctl(0, TIOCGLTC, (char *)&norm_speciax_chars);
    (void)ioctl(0, TIOCGETC, (char *)&norm_tchars);
    (void)ioctl(0, TIOCLGET, (char *)&norm_locax_chars);
#endif
}

/*
 * Save the keymaps (normal and game)
 */
static void keymap_game_prepare(void)
{
#ifdef USE_TPOSIX
    /* Acquire the current mapping */
    tcgetattr(0, &game_termios);

    /* Force "Ctrl-C" to interupt */
    game_termios.c_cc[VINTR] = (char)3;

    /* Force "Ctrl-Z" to suspend */
    game_termios.c_cc[VSUSP] = (char)26;

    /* Hack -- Leave "VSTART/VSTOP" alone */

    /* Disable the standard control characters */
    game_termios.c_cc[VQUIT] = (char)-1;
    game_termios.c_cc[VERASE] = (char)-1;
    game_termios.c_cc[VKILL] = (char)-1;
    game_termios.c_cc[VEOF] = (char)-1;
    game_termios.c_cc[VEOL] = (char)-1;

    /* Normally, block until a character is read */
    game_termios.c_cc[VMIN] = 1;
    game_termios.c_cc[VTIME] = 0;
#endif

#ifdef USE_TERMIO
    /* Acquire the current mapping */
    (void)ioctl(0, TCGETA, (char *)&game_termio);

    /* Force "Ctrl-C" to interupt */
    game_termio.c_cc[VINTR] = (char)3;

    /* Force "Ctrl-Z" to suspend */
    game_termio.c_cc[VSUSP] = (char)26;

    /* Disable the standard control characters */
    game_termio.c_cc[VQUIT] = (char)-1;
    game_termio.c_cc[VERASE] = (char)-1;
    game_termio.c_cc[VKILL] = (char)-1;
    game_termio.c_cc[VEOF] = (char)-1;
    game_termio.c_cc[VEOL] = (char)-1;

    /* Normally, block until a character is read */
    game_termio.c_cc[VMIN] = 1;
    game_termio.c_cc[VTIME] = 0;
#endif

#ifdef USE_TCHARS
    /* Get the default game characters */
    (void)ioctl(0, TIOCGETP, (char *)&game_ttyb);
    (void)ioctl(0, TIOCGLTC, (char *)&game_speciax_chars);
    (void)ioctl(0, TIOCGETC, (char *)&game_tchars);
    (void)ioctl(0, TIOCLGET, (char *)&game_locax_chars);

    /* Force suspend (^Z) */
    game_speciax_chars.t_suspc = (char)26;

    /* Cancel some things */
    game_speciax_chars.t_dsuspc = (char)-1;
    game_speciax_chars.t_rprntc = (char)-1;
    game_speciax_chars.t_flushc = (char)-1;
    game_speciax_chars.t_werasc = (char)-1;
    game_speciax_chars.t_lnextc = (char)-1;

    /* Force interupt (^C) */
    game_tchars.t_intrc = (char)3;

    /* Force start/stop (^Q, ^S) */
    game_tchars.t_startc = (char)17;
    game_tchars.t_stopc = (char)19;

    /* Cancel some things */
    game_tchars.t_quitc = (char)-1;
    game_tchars.t_eofc = (char)-1;
    game_tchars.t_brkc = (char)-1;
#endif
}

/*
 * Suspend/Resume
 */
static errr Term_xtra_gcu_alive(int v)
{
    if (!v) {
        /* Go to normal keymap mode */
        keymap_norm();

        /* Restore modes */
        nocbreak();
        echo();
        nl();

        /* Hack -- make sure the cursor is visible */
        term_xtra(TERM_XTRA_SHAPE, 1);

        /* Flush the curses buffer */
        (void)refresh();

        /* this moves curses to bottom right corner */
        mvcur(getcury(curscr), getcurx(curscr), LINES - 1, 0);

        /* Exit curses */
        endwin();

        /* Flush the output */
        (void)fflush(stdout);
    } else {
        /* Restore the settings */
        cbreak();
        noecho();
        nonl();

        /* Go to angband keymap mode */
        keymap_game();
    }

    return (0);
}

/*
 * Check for existance of a file
 */
static bool check_file(concptr s)
{
    FILE *fff;
    fff = fopen(s, "r");
    if (!fff)
        return false;

    fclose(fff);
    return true;
}

/*
 * Initialize sound
 */
static bool init_sound(void)
{
    /* Initialize once */
    if (can_use_sound)
        return can_use_sound;

    int i;
    char wav[128];
    char buf[1024];

    /* Prepare the sounds */
    for (i = 1; i < SOUND_MAX; i++) {
        /* Extract name of sound file */
        sprintf(wav, "%s.wav", angband_sound_name[i]);

        /* Access the sound */
        path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA_SOUND, wav);

        /* Save the sound filename, if it exists */
        if (check_file(buf))
            sound_file[i] = string_make(buf);
    }

    /* Sound available */
    can_use_sound = true;
    return (can_use_sound);
}

/*
 * Init the "curses" system
 */
static void Term_init_gcu(term_type *t)
{
    term_data *td = (term_data *)(t->data);

    /* Count init's, handle first */
    if (active++ != 0)
        return;

    /* Erase the screen */
    (void)wclear(td->win);

    /* Reset the cursor */
    (void)wmove(td->win, 0, 0);

    /* Flush changes */
    (void)wrefresh(td->win);

    /* Game keymap */
    keymap_game();
}

/*
 * Nuke the "curses" system
 */
static void Term_nuke_gcu(term_type *t)
{
    term_data *td = (term_data *)(t->data);

    /* Delete this window */
    delwin(td->win);

    /* Count nuke's, handle last */
    if (--active != 0)
        return;

    /* Hack -- make sure the cursor is visible */
    term_xtra(TERM_XTRA_SHAPE, 1);

#ifdef A_COLOR
    /* Reset colors to defaults */
    start_color();
#endif

    /* This moves curses to bottom right corner */
    mvcur(getcury(curscr), getcurx(curscr), LINES - 1, 0);

    /* Flush the curses buffer */
    (void)refresh();

    /* Exit curses */
    endwin();

    /* Flush the output */
    (void)fflush(stdout);

    /* Normal keymap */
    keymap_norm();
}

/*
 * Push multiple keys reversal
 */
static void term_string_push(char *buf)
{
    int i, l = strlen(buf);
    for (i = l; i >= 0; i--)
        term_key_push(buf[i]);
}

#ifdef USE_GETCH

/*
 * Process events, with optional wait
 */
static errr Term_xtra_gcu_event(int v)
{
    int i, k;

    /* Wait */
    if (v) {
        char buf[256];
        char *bp = buf;

        /* Paranoia -- Wait for it */
        nodelay(stdscr, false);

        /* Get a keypress */
        i = getch();

        /* Broken input is special */
        if (i == ERR)
            exit_game_panic(p_ptr);
        if (i == EOF)
            exit_game_panic(p_ptr);

        *bp++ = (char)i;

        /* Do not wait for it */
        nodelay(stdscr, true);

        while ((i = getch()) != EOF) {
            if (i == ERR)
                exit_game_panic(p_ptr);
            *bp++ = (char)i;
            if (bp == &buf[255])
                break;
        }

        /* Wait for it next time */
        nodelay(stdscr, false);

        *bp = '\0';
#ifdef JP
        char eucbuf[sizeof(buf)];
        /* strlen + 1 を渡して文字列終端('\0')を含めて変換する */
        if (utf8_to_euc(buf, strlen(buf) + 1, eucbuf, sizeof(eucbuf)) < 0) {
            return (-1);
        }
#endif
        term_string_push(_(eucbuf, buf));
    }

    /* Do not wait */
    else {
        /* Do not wait for it */
        nodelay(stdscr, true);

        /* Check for keypresses */
        i = getch();

        /* Wait for it next time */
        nodelay(stdscr, false);

        /* None ready */
        if (i == ERR)
            return (1);
        if (i == EOF)
            return (1);

        /* Enqueue the keypress */
        term_key_push(i);
    }

    /* Success */
    return (0);
}

#else /* USE_GETCH */

/*
 * Process events (with optional wait)
 */
static errr Term_xtra_gcu_event(int v)
{
    int i, k;

    char buf[256];

    /* Wait */
    if (v) {
        char *bp = buf;

        /* Wait for one byte */
        i = read(0, bp++, 1);

        /* Hack -- Handle bizarre "errors" */
        if ((i <= 0) && (errno != EINTR))
            exit_game_panic(p_ptr);

        /* Get the current flags for stdin */
        k = fcntl(0, F_GETFL, 0);

        /* Oops */
        if (k < 0)
            return (1);

        /* Tell stdin not to block */
        if (fcntl(0, F_SETFL, k | O_NDELAY) >= 0) {
            if ((i = read(0, bp, 254)) > 0) {
                bp += i;
            }

            /* Replace the flags for stdin */
            if (fcntl(0, F_SETFL, k))
                return (1);
        }

        bp[0] = '\0';
#ifdef JP
        char eucbuf[sizeof(buf)];
        /* strlen + 1 を渡して文字列終端('\0')を含めて変換する */
        if (utf8_to_euc(buf, strlen(buf) + 1, eucbuf, sizeof(eucbuf)) < 0) {
            return (-1);
        }
#endif
        term_string_push(_(eucbuf, buf));
    }

    /* Do not wait */
    else {
        /* Get the current flags for stdin */
        k = fcntl(0, F_GETFL, 0);

        /* Oops */
        if (k < 0)
            return (1);

        /* Tell stdin not to block */
        if (fcntl(0, F_SETFL, k | O_NDELAY) < 0)
            return (1);

        /* Read one byte, if possible */
        i = read(0, buf, 1);

        /* Replace the flags for stdin */
        if (fcntl(0, F_SETFL, k))
            return (1);

        /* Ignore "invalid" keys */
        if ((i != 1) || (!buf[0]))
            return (1);

        /* Enqueue the keypress */
        term_key_push(buf[0]);
    }

    /* Success */
    return (0);
}

#endif /* USE_GETCH */

/*
 * Hack -- make a sound
 */
static errr Term_xtra_gcu_sound(int v)
{
    char buf[1024];

    /* Sound disabled */
    if (!use_sound)
        return (1);

    /* Illegal sound */
    if ((v < 0) || (v >= SOUND_MAX))
        return (1);

    /* Unknown sound */
    if (!sound_file[v])
        return (1);

    sprintf(buf, "./gcusound.sh %s\n", sound_file[v]);

    return (system(buf) < 0);

    return (0);
}

static int scale_color(int i, int j, int scale)
{
    return (angband_color_table[i][j] * (scale - 1) + 127) / 255;
}

static int create_color(int i, int scale)
{
    int r = scale_color(i, 1, scale);
    int g = scale_color(i, 2, scale);
    int b = scale_color(i, 3, scale);
    int rgb = 16 + scale * scale * r + scale * g + b;
    /* In the case of white and black we need to use the ANSI colors */
    if (r == g && g == b) {
        if (b == 0)
            rgb = 0;
        if (b == scale)
            rgb = 15;
    }
    return rgb;
}

/*
 * React to changes
 */
static errr Term_xtra_gcu_react(void)
{

#ifdef A_COLOR

    if (!can_change_color()) {
        if (COLORS == 256 || COLORS == 88) {
            /* If we have more than 16 colors, find the best matches. These numbers
             * correspond to xterm/rxvt's builtin color numbers--they do not
             * correspond to curses' constants OR with curses' color pairs.
             *
             * XTerm has 216 (6*6*6) RGB colors, with each RGB setting 0-5.
             * RXVT has 64 (4*4*4) RGB colors, with each RGB setting 0-3.
             *
             * Both also have the basic 16 ANSI colors, plus some extra grayscale
             * colors which we do not use.
             */
            int scale = COLORS == 256 ? 6 : 4;
            for (int i = 0; i < 16; i++) {
                int fg = create_color(i, scale);
                init_pair(i + 1, fg, bg_color);
                colortable[i] = COLOR_PAIR(i + 1) | A_NORMAL;
            }
        }
    } else {
        for (int i = 0; i < 16; ++i) {
            init_color(i,
                (angband_color_table[i][1] * 1000) / 255,
                (angband_color_table[i][2] * 1000) / 255,
                (angband_color_table[i][3] * 1000) / 255);
        }
    }

#endif

    /* Success */
    return (0);
}

/*
 * Handle a "special request"
 */
static errr Term_xtra_gcu(int n, int v)
{
    term_data *td = (term_data *)(Term->data);

    /* Analyze the request */
    switch (n) {
    /* Clear screen */
    case TERM_XTRA_CLEAR:
        touchwin(td->win);
        (void)wclear(td->win);
        return (0);

    /* Make a noise */
    case TERM_XTRA_NOISE:
        return write(1, "\007", 1) != 1;

    /* Make a special sound */
    case TERM_XTRA_SOUND:
        return (Term_xtra_gcu_sound(v));

    /* Flush the Curses buffer */
    case TERM_XTRA_FRESH:
        (void)wrefresh(td->win);
        return (0);

    /* Change the cursor visibility */
    case TERM_XTRA_SHAPE:
        curs_set(v);
        return (0);

    /* Suspend/Resume curses */
    case TERM_XTRA_ALIVE:
        return (Term_xtra_gcu_alive(v));

    /* Process events */
    case TERM_XTRA_EVENT:
        return (Term_xtra_gcu_event(v));

    /* Flush events */
    case TERM_XTRA_FLUSH:
        while (!Term_xtra_gcu_event(false))
            ;
        return (0);

    /* Delay */
    case TERM_XTRA_DELAY:
        usleep(1000 * v);
        return (0);

    /* React to events */
    case TERM_XTRA_REACT:
        Term_xtra_gcu_react();
        return (0);
    }

    /* Unknown */
    return (1);
}

/*
 * Actually MOVE the hardware cursor
 */
static errr Term_curs_gcu(int x, int y)
{
    term_data *td = (term_data *)(Term->data);

    /* Literally move the cursor */
    wmove(td->win, y, x);

    /* Success */
    return (0);
}

/*
 * Erase a grid of space
 * Hack -- try to be "semi-efficient".
 */
static errr Term_wipe_gcu(int x, int y, int n)
{
    term_data *td = (term_data *)(Term->data);

    /* Place cursor */
    wmove(td->win, y, x);

    /* Clear to end of line */
    if (x + n >= 80) {
        wclrtoeol(td->win);
    }

    /* Clear some characters */
    else {
        while (n-- > 0)
            waddch(td->win, ' ');
    }

    /* Success */
    return (0);
}

#ifdef USE_NCURSES_ACS
/*
 * this function draws some ACS characters on the screen
 * for DOS-based users: these are the graphical chars (blocks, lines etc)
 *
 * unix-gurus: before you start adding other attributes like A_REVERSE
 * think hard about how map_info() in cave.c should handle the color
 * of something that we here draw in reverse. It's not so simple, alas.
 */
static void Term_acs_text_gcu(int x, int y, int n, byte a, concptr s)
{
    term_data *td = (term_data *)(Term->data);
    int i;

    /* position the cursor */
    wmove(td->win, y, x);

#ifdef A_COLOR
    /* Set the color */
    wattrset(td->win, colortable[a & 0x0F]);
#endif

    for (i = 0; i < n; i++) {
        /* add acs_map of a */
        waddch(td->win, acs_map[(int)s[i]]);
    }
    wattrset(td->win, WA_NORMAL);
}
#endif

/*
 * Place some text on the screen using an attribute
 */
static errr Term_text_gcu(int x, int y, int n, byte a, concptr s)
{
    term_data *td = (term_data *)(Term->data);

#ifdef USE_NCURSES_ACS
    /* do we have colors + 16 ? */
    /* then call special routine for drawing special characters */
    if (a & 0x10) {
        Term_acs_text_gcu(x, y, n, a, s);
        return (0);
    }
#endif

    /* Move the cursor and dump the string */
    wmove(td->win, y, x);

#ifdef A_COLOR
    /* Set the color */
    if (can_use_color)
        wattrset(td->win, colortable[a & 0x0F]);
#endif

#ifdef JP
    char text[1024];
    int text_len = euc_to_utf8(s, n, text, sizeof(text));
    if (text_len < 0) {
        return (-1);
    }
#endif
    /* Add the text */
    waddnstr(td->win, _(text, s), _(text_len, n));

    /* Success */
    return (0);
}

/**
 * Create a window for the given "term_data" argument.
 *
 * Assumes legal arguments.
 */
static errr term_data_init_gcu(term_data *td, int rows, int cols, int y, int x)
{
    term_type *t = &td->t;

    /* Make sure the window has a positive size */
    if (rows <= 0 || cols <= 0)
        return (0);

    /* Create a window */
    td->win = newwin(rows, cols, y, x);

    /* Make sure we succeed */
    if (!td->win) {
        plog("Failed to setup curses window.");
        return (-1);
    }

    /* Initialize the term */
    term_init(t, cols, rows, 256);

    /* Avoid the bottom right corner */
    t->icky_corner = true;

    /* Erase with "white space" */
    t->attr_blank = TERM_WHITE;
    t->char_blank = ' ';

    /* Set some hooks */
    t->init_hook = Term_init_gcu;
    t->nuke_hook = Term_nuke_gcu;

    /* Set some more hooks */
    t->text_hook = Term_text_gcu;
    t->wipe_hook = Term_wipe_gcu;
    t->curs_hook = Term_curs_gcu;
    t->xtra_hook = Term_xtra_gcu;

    /* Save the data */
    t->data = td;

    /* Activate it */
    term_activate(t);

    /* Success */
    return (0);
}

/**
 * Simple helper
 */
static errr term_data_init(term_data *td)
{
    return term_data_init_gcu(td, td->r.cy, td->r.cx, td->r.y, td->r.x);
}

/* Parse 27,15,*x30 up to the 'x'. * gets converted to a big number
   Parse 32,* until the end. Return count of numbers parsed */
static int _parse_size_list(const char *arg, int sizes[], int max)
{
    int i = 0;
    const char *start = arg;
    const char *stop = arg;

    for (;;) {
        if (!*stop || !isdigit(*stop)) {
            if (i >= max)
                break;
            if (*start == '*')
                sizes[i] = 255;
            else {
                /* rely on atoi("23,34,*") -> 23
                   otherwise, copy [start, stop) into a new buffer first.*/
                sizes[i] = atoi(start);
            }
            i++;
            if (!*stop || *stop != ',')
                break;

            stop++;
            start = stop;
        } else
            stop++;
    }
    return i;
}

static void hook_quit(concptr str)
{
    /* Unused */
    (void)str;

    /* Exit curses */
    endwin();
}

/*
 * Prepare "curses" for use by the file "term.c"
 *
 * Installs the "hook" functions defined above, and then activates
 * the main screen "term", which clears the screen and such things.
 *
 * Someone should really check the semantics of "initscr()"
 */
errr init_gcu(int argc, char *argv[])
{
    int i;

    int num_term = 4, next_win = 0;
    char path[1024];

    /* Unused */
    (void)argc;
    (void)argv;

    setlocale(LC_ALL, "");

    /* Build the "sound" path */
    path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "sound");

    /* Allocate the path */
    ANGBAND_DIR_XTRA_SOUND = string_make(path);

    /* Extract the normal keymap */
    keymap_norm_prepare();

    bool nobigscreen = false;

    /* Parse args */
    for (i = 1; i < argc; i++) {
        if (prefix(argv[i], "-o")) {
            nobigscreen = true;
        }
    }

    /* Initialize for others systems */
    if (initscr() == (WINDOW *)ERR)
        return (-1);

    /* Activate hooks */
    quit_aux = hook_quit;
    core_aux = hook_quit;

    /* Hack -- Require large screen, or Quit with message */
    i = ((LINES < 24) || (COLS < 80));
    if (i)
        quit("Angband needs an 80x24 'curses' screen");

#ifdef A_COLOR

    /*** Init the Color-pairs and set up a translation table ***/

    /* Do we have color, and enough color, available? */
    can_use_color = ((start_color() != ERR) && has_colors() && (COLORS >= 8) && (COLOR_PAIRS >= 8));

#ifdef REDEFINE_COLORS
    /* Can we change colors? */
    can_fix_color = (can_use_color && can_change_color() && (COLORS >= 16) && (COLOR_PAIRS > 8));
#endif

    /* Attempt to use customized colors */
    if (can_fix_color) {
        /* Prepare the color pairs */
        for (i = 1; i <= 15; i++) {
            if (init_pair(i, i, 0) == ERR) {
                quit("Color pair init failed");
            }

            colortable[i] = COLOR_PAIR(i);
            Term_xtra_gcu_react();
        }
    }
    /* Attempt to use colors */
    else if (can_use_color) {
        /* Color-pair 0 is *always* WHITE on BLACK */

        /* Prepare the color pairs */
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_CYAN, COLOR_BLACK);
        init_pair(7, COLOR_BLACK, COLOR_BLACK);

        /* Prepare the "Angband Colors" -- Bright white is too bright */
        /* Changed in Drangband. Cyan as grey sucks -- -TM- */
        colortable[0] = (COLOR_PAIR(7) | A_NORMAL); /* Black */
        colortable[1] = (COLOR_PAIR(0) | A_BRIGHT); /* White */
        colortable[2] = (COLOR_PAIR(0) | A_NORMAL); /* Grey XXX */
        colortable[3] = (COLOR_PAIR(1) | A_BRIGHT); /* Orange XXX */
        colortable[4] = (COLOR_PAIR(1) | A_NORMAL); /* Red */
        colortable[5] = (COLOR_PAIR(2) | A_NORMAL); /* Green */
        colortable[6] = (COLOR_PAIR(4) | A_BRIGHT); /* Blue */
        colortable[7] = (COLOR_PAIR(3) | A_NORMAL); /* Umber */
        colortable[8] = (COLOR_PAIR(7) | A_BRIGHT); /* Dark-grey XXX */
        colortable[9] = (COLOR_PAIR(0) | A_NORMAL); /* Light-grey XXX */
        colortable[10] = (COLOR_PAIR(5) | A_BRIGHT); /* Purple */
        colortable[11] = (COLOR_PAIR(3) | A_BRIGHT); /* Yellow */
        colortable[12] = (COLOR_PAIR(5) | A_NORMAL); /* Light Red XXX */
        colortable[13] = (COLOR_PAIR(2) | A_BRIGHT); /* Light Green */
        colortable[14] = (COLOR_PAIR(6) | A_BRIGHT); /* Light Blue */
        colortable[15] = (COLOR_PAIR(3) | A_NORMAL); /* Light Umber XXX */
    }

#endif

    /* Handle "arg_sound" */
    if (use_sound != arg_sound) {
        /* Initialize (if needed) */
        if (arg_sound && !init_sound()) {
            /* Warning */
            plog("Cannot initialize sound!");

            /* Cannot enable */
            arg_sound = false;
        }

        /* Change setting */
        use_sound = arg_sound;
    }

    /* Try graphics */
    if (arg_graphics) {
        /* if USE_NCURSES_ACS is defined, we can do something with graphics in curses! */
#ifdef USE_NCURSES_ACS
        use_graphics = true;
#endif
    }

    /*** Low level preparation ***/

#ifdef USE_GETCH

    /* Paranoia -- Assume no waiting */
    nodelay(stdscr, false);

#endif

    /* Prepare */
    cbreak();
    noecho();
    nonl();
    raw();

    /* Extract the game keymap */
    keymap_game_prepare();

    /*** Now prepare the term(s) ***/
    if (nobigscreen) {
        /* Create several terms */
        for (i = 0; i < num_term; i++) {
            int rows, cols, y, x;

            /* Decide on size and position */
            switch (i) {
            /* Upper left */
            case 0: {
                rows = 24;
                cols = 80;
                y = x = 0;
                break;
            }

            /* Lower left */
            case 1: {
                rows = LINES - 25;
                cols = 80;
                y = 25;
                x = 0;
                break;
            }

            /* Upper right */
            case 2: {
                rows = 24;
                cols = COLS - 81;
                y = 0;
                x = 81;
                break;
            }

            /* Lower right */
            case 3: {
                rows = LINES - 25;
                cols = COLS - 81;
                y = 25;
                x = 81;
                break;
            }

            /* XXX */
            default: {
                rows = cols = y = x = 0;
                break;
            }
            }

            /* Skip non-existant windows */
            if (rows <= 0 || cols <= 0)
                continue;

            /* Create a term */
            term_data_init_gcu(&data[next_win], rows, cols, y, x);

            /* Remember the term */
            angband_term[next_win] = &data[next_win].t;

            /* One more window */
            next_win++;
        }
    } else
    /* Parse Args and Prepare the Terminals. Rectangles are specified
      as Width x Height, right? The game will allow you to have two
      strips of extra terminals, one on the right and one on the bottom.
      The map terminal will than fit in as big as possible in the remaining
      space.
      Examples:
        angband -mgcu -- -right 30x27,* -bottom *x7 will layout as
        Term-0: Map (COLS-30)x(LINES-7) | Term-1: 30x27
        --------------------------------|----------------------
        <----Term-3: (COLS-30)x7------->| Term-2: 30x(LINES-27)
        composband -mgcu -- -bottom *x7 -right 30x27,* will layout as
        Term-0: Map (COLS-30)x(LINES-7) | Term-2: 30x27
                                        |------------------------------
                                        | Term-3: 30x(LINES-27)
        ---------------------------------------------------------------
        <----------Term-1: (COLS)x7----------------------------------->
        Notice the effect on the bottom terminal by specifying its argument
        second or first. Notice the sequence numbers for the various terminals
        as you will have to blindly configure them in the window setup screen.
        EDIT: Added support for -left and -top.
    */
    {
        rect_t remaining = rect(0, 0, COLS, LINES);
        int spacer_cx = 1;
        int spacer_cy = 1;
        int next_term = 1;
        int term_ct = 1;

        for (i = 1; i < argc; i++) {
            if (streq(argv[i], "-spacer")) {
                i++;
                if (i >= argc)
                    quit("Missing size specifier for -spacer");
                sscanf(argv[i], "%dx%d", &spacer_cx, &spacer_cy);
            } else if (streq(argv[i], "-right") || streq(argv[i], "-left")) {
                const char *arg, *tmp;
                bool left = streq(argv[i], "-left");
                int cx, cys[MAX_TERM_DATA] = { 0 }, ct, j, x, y;

                i++;
                if (i >= argc)
                    quit(format("Missing size specifier for -%s", left ? "left" : "right"));

                arg = argv[i];
                tmp = strchr(arg, 'x');
                if (!tmp)
                    quit(format("Expected something like -%s 60x27,* for two %s hand terminals of 60 columns, the first 27 lines and the second whatever is left.", left ? "left" : "right", left ? "left" : "right"));
                cx = atoi(arg);
                remaining.cx -= cx;
                if (left) {
                    x = remaining.x;
                    y = remaining.y;
                    remaining.x += cx;
                } else {
                    x = remaining.x + remaining.cx;
                    y = remaining.y;
                }
                remaining.cx -= spacer_cx;
                if (left)
                    remaining.x += spacer_cx;

                tmp++;
                ct = _parse_size_list(tmp, cys, MAX_TERM_DATA);
                for (j = 0; j < ct; j++) {
                    int cy = cys[j];
                    if (y + cy > remaining.y + remaining.cy)
                        cy = remaining.y + remaining.cy - y;
                    if (next_term >= MAX_TERM_DATA)
                        quit(format("Too many terminals. Only %d are allowed.", MAX_TERM_DATA));
                    if (cy <= 0) {
                        quit(format("Out of bounds in -%s: %d is too large (%d rows max for this strip)",
                            left ? "left" : "right", cys[j], remaining.cy));
                    }
                    data[next_term++].r = rect(x, y, cx, cy);
                    y += cy + spacer_cy;
                    term_ct++;
                }
            } else if (streq(argv[i], "-top") || streq(argv[i], "-bottom")) {
                const char *arg, *tmp;
                bool top = streq(argv[i], "-top");
                int cy, cxs[MAX_TERM_DATA] = { 0 }, ct, j, x, y;

                i++;
                if (i >= argc)
                    quit(format("Missing size specifier for -%s", top ? "top" : "bottom"));

                arg = argv[i];
                tmp = strchr(arg, 'x');
                if (!tmp)
                    quit(format("Expected something like -%s *x7 for a single %s terminal of 7 lines using as many columns as are available.", top ? "top" : "bottom", top ? "top" : "bottom"));
                tmp++;
                cy = atoi(tmp);
                ct = _parse_size_list(arg, cxs, MAX_TERM_DATA);

                remaining.cy -= cy;
                if (top) {
                    x = remaining.x;
                    y = remaining.y;
                    remaining.y += cy;
                } else {
                    x = remaining.x;
                    y = remaining.y + remaining.cy;
                }
                remaining.cy -= spacer_cy;
                if (top)
                    remaining.y += spacer_cy;

                tmp++;
                for (j = 0; j < ct; j++) {
                    int cx = cxs[j];
                    if (x + cx > remaining.x + remaining.cx)
                        cx = remaining.x + remaining.cx - x;
                    if (next_term >= MAX_TERM_DATA)
                        quit(format("Too many terminals. Only %d are allowed.", MAX_TERM_DATA));
                    if (cx <= 0) {
                        quit(format("Out of bounds in -%s: %d is too large (%d cols max for this strip)",
                            top ? "top" : "bottom", cxs[j], remaining.cx));
                    }
                    data[next_term++].r = rect(x, y, cx, cy);
                    x += cx + spacer_cx;
                    term_ct++;
                }
            }
        }

        /* Map Terminal */
        if (remaining.cx < MIN_TERM0_COLS || remaining.cy < MIN_TERM0_LINES)
            quit(format("Failed: angband needs an %dx%d map screen, not %dx%d", MIN_TERM0_COLS, MIN_TERM0_LINES, remaining.cx, remaining.cy));
        data[0].r = remaining;
        term_data_init(&data[0]);
        angband_term[0] = Term;

        /* Child Terminals */
        for (next_term = 1; next_term < term_ct; next_term++) {
            term_data_init(&data[next_term]);
            angband_term[next_term] = Term;
        }
    }

    /* Activate the "Angband" window screen */
    term_activate(&data[0].t);

    /* Store */
    term_screen = &data[0].t;

    /* Success */
    return (0);
}

#endif /* USE_GCU */
