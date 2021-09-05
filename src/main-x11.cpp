/* File: main-x11.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/*
 * This file helps Angband work with UNIX/X11 computers.
 *
 * To use this file, compile with "USE_X11" defined, and link against all
 * the various "X11" libraries which may be needed.
 *
 * See also "main-xaw.c".
 *
 * Part of this file provides a user interface package composed of several
 * pseudo-objects, including "metadpy" (a display), "infowin" (a window),
 * "infoclr" (a color), and "infofnt" (a font).  Actually, the package was
 * originally much more interesting, but it was bastardized to keep this
 * file simple.
 *
 * The rest of this file is an implementation of "main-xxx.c" for X11.
 *
 * Most of this file is by Ben Harrison (benh@phial.com).
 */

/*
 * The following shell script can be used to launch Angband, assuming that
 * it was extracted into "~/Angband", and compiled using "USE_X11", on a
 * Linux machine, with a 1280x1024 screen, using 6 windows (with the given
 * characteristics), with gamma correction of 1.8 -> (1 / 1.8) * 256 = 142,
 * and without graphics (add "-g" for graphics).  Just copy this comment
 * into a file, remove the leading " * " characters (and the head/tail of
 * this comment), and make the file executable.
 *
 *
 * #!/bin/csh
 *
 * # Describe attempt
 * echo "Launching angband..."
 * sleep 2
 *
 * # Main window
 * setenv ANGBAND_X11_FONT_0 10x20
 * setenv ANGBAND_X11_AT_X_0 5
 * setenv ANGBAND_X11_AT_Y_0 510
 *
 * # Message window
 * setenv ANGBAND_X11_FONT_1 8x13
 * setenv ANGBAND_X11_AT_X_1 5
 * setenv ANGBAND_X11_AT_Y_1 22
 * setenv ANGBAND_X11_ROWS_1 35
 *
 * # Inventory window
 * setenv ANGBAND_X11_FONT_2 8x13
 * setenv ANGBAND_X11_AT_X_2 635
 * setenv ANGBAND_X11_AT_Y_2 182
 * setenv ANGBAND_X11_ROWS_3 23
 *
 * # Equipment window
 * setenv ANGBAND_X11_FONT_3 8x13
 * setenv ANGBAND_X11_AT_X_3 635
 * setenv ANGBAND_X11_AT_Y_3 22
 * setenv ANGBAND_X11_ROWS_3 12
 *
 * # Monster recall window
 * setenv ANGBAND_X11_FONT_4 6x13
 * setenv ANGBAND_X11_AT_X_4 817
 * setenv ANGBAND_X11_AT_Y_4 847
 * setenv ANGBAND_X11_COLS_4 76
 * setenv ANGBAND_X11_ROWS_4 11
 *
 * # Object recall window
 * setenv ANGBAND_X11_FONT_5 6x13
 * setenv ANGBAND_X11_AT_X_5 817
 * setenv ANGBAND_X11_AT_Y_5 520
 * setenv ANGBAND_X11_COLS_5 76
 * setenv ANGBAND_X11_ROWS_5 24
 *
 * # The build directory
 * cd ~/Angband
 *
 * # Gamma correction
 * setenv ANGBAND_X11_GAMMA 142
 *
 * # Launch Angband
 * ./src/angband -mx11 -- -n6 &
 *
 */

#include "cmd-io/macro-util.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "io/files-util.h"
#include "locale/japanese.h"
#include "locale/utf-8.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "main/x11-type-string.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"

/*
 * Available graphic modes
 */
// clang-format off
#define GRAPHICS_NONE       0
#define GRAPHICS_ORIGINAL   1
#define GRAPHICS_ADAM_BOLT  2
#define GRAPHICS_HENGBAND   3
// clang-format on

#ifdef USE_X11
#ifndef __MAKEDEPEND__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#ifdef USE_LOCALE
#include <X11/Xlocale.h>
#endif
#include <X11/Xatom.h>
#endif /* __MAKEDEPEND__ */

#ifdef USE_XFT
#include <X11/Xft/Xft.h>
#endif

/*
 * Include some helpful X11 code.
 */
#include "maid-x11.cpp"

/*
 * Notes on Colors:
 *
 *   1) On a monochrome (or "fake-monochrome") display, all colors
 *   will be "cast" to "fg," except for the bg color, which is,
 *   obviously, cast to "bg".  Thus, one can ignore this setting.
 *
 *   2) Because of the inner functioning of the color allocation
 *   routines, colors may be specified as (a) a typical color name,
 *   (b) a hexidecimal color specification (preceded by a pound sign),
 *   or (c) by strings such as "fg", "bg", "zg".
 *
 *   3) Due to the workings of the init routines, many colors
 *   may also be dealt with by their actual pixel values.  Note that
 *   the pixel with all bits set is "zg = (1<<metadpy->depth)-1", which
 *   is not necessarily either black or white.
 */

/**** Generic Types ****/

/*
 * An X11 pixell specifier
 */
#ifdef USE_XFT
typedef XftColor Pixell;
#else
typedef unsigned long Pixell;
#endif

/*
 * A structure summarizing a given Display.
 *
 *	- The Display itself
 *	- The default Screen for the display
 *	- The virtual root (usually just the root)
 *	- The default colormap (from a macro)
 *
 *	- The "name" of the display
 *
 *	- The socket to listen to for events
 *
 *	- The width of the display screen (from a macro)
 *	- The height of the display screen (from a macro)
 *	- The bit depth of the display screen (from a macro)
 *
 *	- The black Pixell (from a macro)
 *	- The white Pixell (from a macro)
 *
 *	- The background Pixell (default: black)
 *	- The foreground Pixell (default: white)
 *	- The maximal Pixell (Equals: ((2 ^ depth)-1), is usually ugly)
 *
 *	- Bit Flag: Force all colors to black and white (default: !color)
 *	- Bit Flag: Allow the use of color (default: depth > 1)
 *	- Bit Flag: We created 'dpy', and so should nuke it when done.
 */
struct metadpy {
    Display *dpy;
    Screen *screen;
    Window root;
    Colormap cmap;
#ifdef USE_XIM
    XIM xim;
#endif

    char *name;

    int fd;

    uint width;
    uint height;
    uint depth;

    Pixell black;
    Pixell white;

    Pixell bg;
    Pixell fg;
#ifndef USE_XFT
    Pixell zg;
#endif

    uint mono : 1;
    uint color : 1;
    uint nuke : 1;
};

/*
 * A Structure summarizing Window Information.
 *
 * I assume that a window is at most 30000 pixels on a side.
 * I assume that the root windw is also at most 30000 square.
 *
 *	- The Window
 *	- The current Input Event Mask
 *
 *	- The location of the window
 *	- The width, height of the window
 *	- The border width of this window
 *
 *	- Byte: 1st Extra byte
 *
 *	- Bit Flag: This window is currently Mapped
 *	- Bit Flag: This window needs to be redrawn
 *	- Bit Flag: This window has been resized
 *
 *	- Bit Flag: We should nuke 'win' when done with it
 *
 *	- Bit Flag: 1st extra flag
 *	- Bit Flag: 2nd extra flag
 *	- Bit Flag: 3rd extra flag
 *	- Bit Flag: 4th extra flag
 */
struct infowin {
    Window win;
#ifdef USE_XIM
    XIC xic;
    long xic_mask;
#endif
#ifdef USE_XFT
    XftDraw *draw;
#endif

    long mask;

    int16_t ox, oy;
    int16_t x, y;
    int16_t w, h;
    uint16_t b;

    byte byte1;

    uint mapped : 1;
    uint redraw : 1;
    uint resize : 1;

    uint nuke : 1;

    uint flag1 : 1;
    uint flag2 : 1;
    uint flag3 : 1;
    uint flag4 : 1;
};

/*
 * A Structure summarizing Operation+Color Information
 *
 *	- The actual GC corresponding to this info
 *
 *	- The Foreground Pixell Value
 *	- The Background Pixell Value
 *
 *	- Num (0-15): The operation code (As in Clear, Xor, etc)
 *	- Bit Flag: The GC is in stipple mode
 *	- Bit Flag: Destroy 'gc' at Nuke time.
 */
struct infoclr {
#ifndef USE_XFT
    GC gc;
#endif

    Pixell fg;
    Pixell bg;

    uint code : 4;
    uint stip : 1;
    uint nuke : 1;
};

/*
 * A Structure to Hold Font Information
 *
 *	- The 'XFontStruct*' (yields the 'Font')
 *
 *	- The font name
 *
 *	- The default character width
 *	- The default character height
 *	- The default character ascent
 *
 *	- Byte: Pixel offset used during fake mono
 *
 *	- Flag: Force monospacing via 'wid'
 *	- Flag: Nuke info when done
 */
struct infofnt {
#ifdef USE_XFT
    XftFont *info;
#else
    XFontSet info;
#endif
    concptr name;

    int16_t wid;
    int16_t twid;
    int16_t hgt;
    int16_t asc;

    byte off;

    uint mono : 1;
    uint nuke : 1;
};

/* Set current metadpy (Metadpy) to 'M' */
#define Metadpy_set(M) Metadpy = M

/* Initialize 'M' using Display 'D' */
#define Metadpy_init_dpy(D) Metadpy_init_2(D, cnullptr)

/* Initialize 'M' using a Display named 'N' */
#define Metadpy_init_name(N) Metadpy_init_2((Display *)(nullptr), N)

/* Initialize 'M' using the standard Display */
#define Metadpy_init() Metadpy_init_name("")

/* Init an infowin by giving father as an (info_win*) (or nullptr), and data */
#define Infowin_init_dad(D, X, Y, W, H, B, FG, BG) Infowin_init_data(((D) ? ((D)->win) : (Window)(None)), X, Y, W, H, B, FG, BG)

/* Init a top level infowin by pos,size,bord,Colors */
#define Infowin_init_top(X, Y, W, H, B, FG, BG) Infowin_init_data(None, X, Y, W, H, B, FG, BG)

/* Request a new standard window by giving Dad infowin and X,Y,W,H */
#define Infowin_init_std(D, X, Y, W, H, B) Infowin_init_dad(D, X, Y, W, H, B, Metadpy->fg, Metadpy->bg)

/* Set the current Infowin */
#define Infowin_set(I) (Infowin = (I))

/* Set the current Infoclr */
#define Infoclr_set(C) (Infoclr = (C))

#define Infoclr_init_ppo(F, B, O, M) Infoclr_init_data(F, B, O, M)

#define Infoclr_init_cco(F, B, O, M) Infoclr_init_ppo(Infoclr_Pixell(F), Infoclr_Pixell(B), O, M)

#define Infoclr_init_ppn(F, B, O, M) Infoclr_init_ppo(F, B, Infoclr_Opcode(O), M)

#define Infoclr_init_ccn(F, B, O, M) Infoclr_init_cco(F, B, Infoclr_Opcode(O), M)

/* Set the current infofnt */
#define Infofnt_set(I) (Infofnt = (I))

/* Errr: Expose Infowin */
#define Infowin_expose() (!(Infowin->redraw = 1))

/* Errr: Unxpose Infowin */
#define Infowin_unexpose() (Infowin->redraw = 0)

/*
 * The "default" values
 */
static metadpy metadpy_default;

/*
 * The "current" variables
 */
static metadpy *Metadpy = &metadpy_default;
static infowin *Infowin = (infowin *)(nullptr);
#ifdef USE_XIM
static infowin *Focuswin = (infowin *)(nullptr);
#endif
static infoclr *Infoclr = (infoclr *)(nullptr);
static infofnt *Infofnt = (infofnt *)(nullptr);

/*
 * Init the current metadpy, with various initialization stuff.
 *
 * Inputs:
 *	dpy:  The Display* to use (if nullptr, create it)
 *	name: The name of the Display (if nullptr, the current)
 *
 * Notes:
 *	If 'name' is nullptr, but 'dpy' is set, extract name from dpy
 *	If 'dpy' is nullptr, then Create the named Display
 *	If 'name' is nullptr, and so is 'dpy', use current Display
 *
 * Return -1 if no Display given, and none can be opened.
 */
static errr Metadpy_init_2(Display *dpy, concptr name)
{
    metadpy *m = Metadpy;
    if (!dpy) {
        dpy = XOpenDisplay(name);
        if (!dpy)
            return (-1);

        m->nuke = 1;
    } else {
        m->nuke = 0;
    }

    m->dpy = dpy;
    m->screen = DefaultScreenOfDisplay(dpy);
    m->root = RootWindowOfScreen(m->screen);
    m->cmap = DefaultColormapOfScreen(m->screen);
    m->name = DisplayString(dpy);
    m->fd = ConnectionNumber(Metadpy->dpy);
    m->width = WidthOfScreen(m->screen);
    m->height = HeightOfScreen(m->screen);
    m->depth = DefaultDepthOfScreen(m->screen);

#ifdef USE_XFT
    Visual *vis = DefaultVisual(dpy, 0);
    XftColorAllocName(dpy, vis, m->cmap, "black", &m->black);
    XftColorAllocName(dpy, vis, m->cmap, "white", &m->white);
#else
    m->black = BlackPixelOfScreen(m->screen);
    m->white = WhitePixelOfScreen(m->screen);
#endif

    m->bg = m->black;
    m->fg = m->white;

#ifndef USE_XFT
    m->zg = (1 << m->depth) - 1;
#endif

    m->color = ((m->depth > 1) ? 1 : 0);
    m->mono = ((m->color) ? 0 : 1);
    return (0);
}

/*
 * General Flush/ Sync/ Discard routine
 */
static errr Metadpy_update(int flush, int sync, int discard)
{
    if (flush)
        XFlush(Metadpy->dpy);
    if (sync)
        XSync(Metadpy->dpy, discard);

    return (0);
}

/*
 * Make a simple beep
 */
static errr Metadpy_do_beep(void)
{
    XBell(Metadpy->dpy, 100);
    return (0);
}

/*
 * Set the name (in the title bar) of Infowin
 */
static errr Infowin_set_name(concptr name)
{
    Status st;
    XTextProperty tp;
    char buf[128];
    char *bp = buf;
    strcpy(buf, name);
    st = XStringListToTextProperty(&bp, 1, &tp);
    if (st) {
        XSetWMName(Metadpy->dpy, Infowin->win, &tp);
        XFree(tp.value);
    }
    return (0);
}

/*
 * Prepare a new 'infowin'.
 */
static errr Infowin_prepare(Window xid)
{
    infowin *iwin = Infowin;
    Window tmp_win;
    XWindowAttributes xwa;
    int x, y;
    unsigned int w, h, b, d;
    iwin->win = xid;
    XGetGeometry(Metadpy->dpy, xid, &tmp_win, &x, &y, &w, &h, &b, &d);

#ifdef USE_XFT
    Visual *vis = DefaultVisual(Metadpy->dpy, 0);
    if (vis->c_class != TrueColor) {
        quit_fmt("Display does not support truecolor.\n");
    }
    iwin->draw = XftDrawCreate(Metadpy->dpy, iwin->win, vis, Metadpy->cmap);
#endif

    iwin->x = x;
    iwin->y = y;
    iwin->w = w;
    iwin->h = h;
    iwin->b = b;

    XGetWindowAttributes(Metadpy->dpy, xid, &xwa);
    iwin->mask = xwa.your_event_mask;
    iwin->mapped = ((xwa.map_state == IsUnmapped) ? 0 : 1);
    iwin->redraw = 1;
    return (0);
}

/*
 * Init an infowin by giving some data.
 *
 * Inputs:
 *	dad: The Window that should own this Window (if any)
 *	x,y: The position of this Window
 *	w,h: The size of this Window
 *	b,d: The border width and pixel depth
 *
 * Notes:
 *	If 'dad == None' assume 'dad == root'
 */
static errr Infowin_init_data(Window dad, int x, int y, int w, int h, int b, Pixell fg, Pixell bg)
{
    Window xid;
    (void)WIPE(Infowin, infowin);
    if (dad == None)
        dad = Metadpy->root;

#ifdef USE_XFT
    xid = XCreateSimpleWindow(Metadpy->dpy, dad, x, y, w, h, b, fg.pixel, bg.pixel);
#else
    xid = XCreateSimpleWindow(Metadpy->dpy, dad, x, y, w, h, b, fg, bg);
#endif

    XSelectInput(Metadpy->dpy, xid, 0L);
    Infowin->nuke = 1;
    return (Infowin_prepare(xid));
}

/*
 * Modify the event mask of an Infowin
 */
static errr Infowin_set_mask(long mask)
{
    Infowin->mask = mask;
    XSelectInput(Metadpy->dpy, Infowin->win, Infowin->mask);
    return (0);
}

/*
 * Request that Infowin be mapped
 */
static errr Infowin_map(void)
{
    XMapWindow(Metadpy->dpy, Infowin->win);
    return (0);
}

/*
 * Request that Infowin be raised
 */
static errr Infowin_raise(void)
{
    XRaiseWindow(Metadpy->dpy, Infowin->win);
    return (0);
}

/*
 * Request that Infowin be moved to a new location
 */
static errr Infowin_impell(int x, int y)
{
    XMoveWindow(Metadpy->dpy, Infowin->win, x, y);
    return (0);
}

/*
 * Resize an infowin
 */
static errr Infowin_resize(int w, int h)
{
    XResizeWindow(Metadpy->dpy, Infowin->win, w, h);
    return (0);
}

/*
 * Visually clear Infowin
 */
static errr Infowin_wipe(void)
{
    XClearWindow(Metadpy->dpy, Infowin->win);
    return (0);
}

/*
 * A nullptr terminated pair list of legal "operation names"
 *
 * Pairs of values, first is texttual name, second is the string
 * holding the decimal value that the operation corresponds to.
 */
// clang-format off
static concptr opcode_pairs[] =
{
    "cpy", "3",
    "xor", "6",
    "and", "1",
    "ior", "7",
    "nor", "8",
    "inv", "10",
    "clr", "0",
    "set", "15",

    "src", "3",
    "dst", "5",

    "+andReverse", "2",
    "+andInverted", "4",
    "+noop", "5",
    "+equiv", "9",
    "+orReverse", "11",
    "+copyInverted", "12",
    "+orInverted", "13",
    "+nand", "14",
    nullptr
};
// clang-format on

/*
 * Parse a word into an operation "code"
 *
 * Inputs:
 *	str: A string, hopefully representing an Operation
 *
 * Output:
 *	0-15: if 'str' is a valid Operation
 *	-1:   if 'str' could not be parsed
 */
static int Infoclr_Opcode(concptr str)
{
    int i;
    for (i = 0; opcode_pairs[i * 2]; ++i) {
        if (streq(opcode_pairs[i * 2], str)) {
            return (atoi(opcode_pairs[i * 2 + 1]));
        }
    }

    return (-1);
}

/*
 * Initialize an infoclr with some data
 *
 * Inputs:
 *	fg:   The Pixell for the requested Foreground (see above)
 *	bg:   The Pixell for the requested Background (see above)
 *	op:   The Opcode for the requested Operation (see above)
 *	stip: The stipple mode
 */
static errr Infoclr_init_data(Pixell fg, Pixell bg, int op, int stip)
{
    infoclr *iclr = Infoclr;

#ifndef USE_XFT
    GC gc;
    XGCValues gcv;
    unsigned long gc_mask;
#endif

#ifndef USE_XFT
    if (bg > Metadpy->zg)
        return (-1);
    if (fg > Metadpy->zg)
        return (-1);
    if ((op < 0) || (op > 15))
        return (-1);

    gcv.function = op;
    gcv.background = bg;
    gcv.foreground = fg;
    if (op == 6)
        gcv.background = 0;
    if (op == 6)
        gcv.foreground = (bg ^ fg);

    gcv.fill_style = (stip ? FillStippled : FillSolid);
    gcv.graphics_exposures = False;
    gc_mask = (GCFunction | GCBackground | GCForeground | GCFillStyle | GCGraphicsExposures);
    gc = XCreateGC(Metadpy->dpy, Metadpy->root, gc_mask, &gcv);
#endif

    (void)WIPE(iclr, infoclr);

#ifndef USE_XFT
    iclr->gc = gc;
#endif

    iclr->nuke = 1;
    iclr->fg = fg;
    iclr->bg = bg;
    iclr->code = op;
    iclr->stip = stip ? 1 : 0;
    return (0);
}

/*
 * Change the 'fg' for an infoclr
 *
 * Inputs:
 *	fg:   The Pixell for the requested Foreground (see above)
 */
static errr Infoclr_change_fg(Pixell fg)
{
    infoclr *iclr = Infoclr;

#ifdef USE_XFT
    iclr->fg = fg;
#else
    if (fg > Metadpy->zg)
        return (-1);

    XSetForeground(Metadpy->dpy, iclr->gc, fg);
#endif

    return (0);
}

/*
 * Prepare a new 'infofnt'
 */
#ifdef USE_XFT
static errr Infofnt_prepare(XftFont *info)
#else
static errr Infofnt_prepare(XFontSet info)
#endif
{
    infofnt *ifnt = Infofnt;

#ifndef USE_XFT
    XCharStruct *cs;
    XFontStruct **fontinfo;
    char **fontname;
    int n_fonts;
    int ascent, descent, width;
#endif

    ifnt->info = info;

#ifdef USE_XFT
    ifnt->asc = info->ascent;
    ifnt->hgt = info->height;
    const char *text = "A";
    XGlyphInfo extent;
    XftTextExtentsUtf8(Metadpy->dpy, info, (FcChar8 *)text, strlen(text), &extent);
    ifnt->wid = extent.xOff;
#else
    n_fonts = XFontsOfFontSet(info, &fontinfo, &fontname);

    ascent = descent = width = 0;
    while (n_fonts-- > 0) {
        cs = &((*fontinfo)->max_bounds);
        if (ascent < (*fontinfo)->ascent)
            ascent = (*fontinfo)->ascent;
        if (descent < (*fontinfo)->descent)
            descent = (*fontinfo)->descent;
        if (((*fontinfo)->max_byte1) > 0) {
            /* 多バイト文字の場合は幅半分(端数切り上げ)で評価する */
            if (width < (cs->width + 1) / 2)
                width = (cs->width + 1) / 2;
        } else {
            if (width < cs->width)
                width = cs->width;
        }
        fontinfo++;
        fontname++;
    }
    ifnt->asc = ascent;
    ifnt->hgt = ascent + descent;
    ifnt->wid = width;
#endif

    if (use_bigtile)
        ifnt->twid = 2 * ifnt->wid;
    else
        ifnt->twid = ifnt->wid;

    return (0);
}

/*
 * Init an infofnt by its Name
 *
 * Inputs:
 *	name: The name of the requested Font
 */
static void Infofnt_init_data(concptr name)

{
#ifdef USE_XFT
    XftFont *info;
#else
    XFontSet info;
    char **missing_list;
    int missing_count;
    char *default_font;
#endif

    if (!name || !*name)
        quit("Missing font!");

#ifdef USE_XFT
    info = XftFontOpenName(Metadpy->dpy, 0, name);
    /* TODO: error handling */
#else
    info = XCreateFontSet(Metadpy->dpy, name, &missing_list, &missing_count, &default_font);
    if (missing_count > 0) {
        printf("missing font(s): \n");
        while (missing_count-- > 0) {
            printf("\t%s\n", missing_list[missing_count]);
        }
        XFreeStringList(missing_list);
    }
#endif

    if (!info)
        quit_fmt("Failed to find font:\"%s\"", name);

    (void)WIPE(Infofnt, infofnt);
    if (Infofnt_prepare(info)) {
#ifdef USE_XFT
        XftFontClose(Metadpy->dpy, info);
#else
        XFreeFontSet(Metadpy->dpy, info);
#endif
        quit_fmt("Failed to prepare font:\"%s\"", name);
    }

    Infofnt->name = string_make(name);
    Infofnt->nuke = 1;
}

#ifdef USE_XFT
static void Infofnt_text_std_xft_draw_str(int x, int y, concptr str, concptr str_end)
{
    int offset = 0;
    while (str < str_end) {
        const int byte_len = utf8_next_char_byte_length(str);

        if (byte_len == 0 || str + byte_len > str_end) {
            return;
        }

        XftDrawStringUtf8(Infowin->draw, &Infoclr->fg, Infofnt->info, x + Infofnt->wid * offset, y, (const FcChar8 *)str, byte_len);
        offset += (byte_len > 1 ? 2 : 1);
        str += byte_len;
    }
}
#endif

/*
 * Standard Text
 */
static errr Infofnt_text_std(int x, int y, concptr str, int len)
{
    if (!str || !*str)
        return (-1);

    if (len < 0)
        len = strlen(str);

    y = (y * Infofnt->hgt) + Infofnt->asc + Infowin->oy;
    x = (x * Infofnt->wid) + Infowin->ox;
    if (Infofnt->mono) {
#ifndef USE_XFT
        int i;
        for (i = 0; i < len; ++i) {
            XDrawImageString(Metadpy->dpy, Infowin->win, Infoclr->gc, x + i * Infofnt->wid + Infofnt->off, y, str + i, 1);
        }
#endif
    } else {
#ifdef JP
        char utf8_buf[1024];
        int utf8_len = euc_to_utf8(str, len, utf8_buf, sizeof(utf8_buf));
        if (utf8_len < 0) {
            return (-1);
        }
#endif

#ifdef USE_XFT
        XftDraw *draw = Infowin->draw;

        XRectangle r;
        r.x = 0;
        r.y = 0;
        r.width = Infofnt->wid * len;
        r.height = Infofnt->hgt;
        XftDrawSetClipRectangles(draw, x, y - Infofnt->asc, &r, 1);
        XftDrawRect(draw, &Infoclr->bg, x, y - Infofnt->asc, Infofnt->wid * len, Infofnt->hgt);
        Infofnt_text_std_xft_draw_str(x, y, _(utf8_buf, str), _(utf8_buf + utf8_len, str + len));
        XftDrawSetClip(draw, 0);
#else
        XmbDrawImageString(Metadpy->dpy, Infowin->win, Infofnt->info, Infoclr->gc, x, y, _(utf8_buf, str), _(utf8_len, len));
#endif
    }

    return (0);
}

/*
 * Painting where text would be
 */
static errr Infofnt_text_non(int x, int y, concptr str, int len)
{
    int w, h;
    if (len < 0)
        len = strlen(str);

    w = len * Infofnt->wid;
    x = x * Infofnt->wid + Infowin->ox;
    h = Infofnt->hgt;
    y = y * h + Infowin->oy;

#ifdef USE_XFT
    XftDrawRect(Infowin->draw, &Infoclr->fg, x, y, w, h);
#else
    XFillRectangle(Metadpy->dpy, Infowin->win, Infoclr->gc, x, y, w, h);
#endif

    return (0);
}

/*
 * Angband specific code follows... (ANGBAND)
 */

/*
 * Hack -- cursor color
 */
static infoclr *xor_;

/*
 * Actual color table
 */
static infoclr *clr[256];

/*
 * Color info (unused, red, green, blue).
 */
static byte color_table[256][4];

/*
 * A structure for each "term"
 */
struct term_data {
    term_type t;
    infofnt *fnt;
    infowin *win;
#ifndef USE_XFT
    XImage *tiles;
    XImage *TmpImage;
#endif
};

/*
 * The number of term data structures
 */
#define MAX_TERM_DATA 8

/*
 * The array of term data structures
 */
static term_data data[MAX_TERM_DATA];

/* Use short names for the most commonly used elements of various structures. */
#define DPY (Metadpy->dpy)
#define WIN (Infowin->win)

/* Describe a set of co-ordinates. */
struct co_ord {
    int x;
    int y;
};

/*
 * A special structure to store information about the text currently
 * selected.
 */
struct x11_selection_type {
    bool select; /* The selection is currently in use. */
    bool drawn; /* The selection is currently displayed. */
    term_type *t; /* The window where the selection is found. */
    co_ord init; /* The starting co-ordinates. */
    co_ord cur; /* The end co-ordinates (the current ones if still copying). */
    co_ord old; /* The previous end co-ordinates. */
    Time time; /* The time at which the selection was finalised. */
};

static x11_selection_type s_ptr[1];

// ゲーム側へキーを送る
static void send_key(const char key)
{
    // Windows ドライバと同様、自前でキューを操作する。
    // 逆順に term_key_push() する方法だと長い日本語を入力したときにテキストの
    // 順序が入れ替わってしまう。

    // キーバッファが一杯なら入力を捨てる
    const int head_nxt = Term->key_head + 1 == Term->key_size ? 0 : Term->key_head + 1;
    if (head_nxt == Term->key_tail) {
        plog_fmt("key buffer overflow, ignoring key 0x%02X", key);
        return;
    }

    Term->key_queue[Term->key_head] = key;
    Term->key_head = head_nxt;
}

// ゲーム側へキー列を送る
static void send_keys(const char *const keys)
{
    for (const char *p = keys; *p != '\0'; ++p)
        send_key(*p);
}

/*
 * Process a keypress event
 *
 * Also appears in "main-xaw.c".
 */
static void react_keypress(XKeyEvent *xev)
{
    int n, mc, ms, mo, mx;
    uint ks1;
    XKeyEvent *ev = (XKeyEvent *)(xev);
    KeySym ks;
    char buf[128];
    char msg[128];

#ifdef USE_XIM
    int valid_keysym = true;
#endif

#ifdef USE_XIM
    if (Focuswin && Focuswin->xic) {
        Status status;
        n = XmbLookupString(Focuswin->xic, ev, buf, 125, &ks, &status);
        if (status == XBufferOverflow) {
            printf("Input is too long, and dropped\n");
            return;
        }
        if (status != XLookupKeySym && status != XLookupBoth) {
            valid_keysym = false;
        }
    } else {
        n = XLookupString(ev, buf, 125, &ks, nullptr);
    }
#else
    n = XLookupString(ev, buf, 125, &ks, nullptr);
#endif

    buf[n] = '\0';

#ifdef USE_XIM
    if (!valid_keysym) { /* XIMからの入力時のみ false になる */
#ifdef JP
        char euc_buf[sizeof(buf)];
        /* strlen + 1 を渡して文字列終端('\0')を含めて変換する */
        if (utf8_to_euc(buf, strlen(buf) + 1, euc_buf, sizeof(euc_buf)) < 0) {
            return;
        }
#endif
        send_keys(_(euc_buf, buf));
        return;
    }
#endif

    if (IsModifierKey(ks))
        return;

    ks1 = (uint)(ks);
    mc = any_bits(ev->state, ControlMask);
    ms = any_bits(ev->state, ShiftMask);
    mo = any_bits(ev->state, Mod1Mask);
    mx = any_bits(ev->state, Mod2Mask);
    if (n && !mo && !mx && !IsSpecialKey(ks)) {
        send_keys(buf);
        return;
    }

    switch (ks1) {
    case XK_Escape: {
        send_key(ESCAPE);
        return;
    }

    case XK_Return: {
        send_key('\r');
        return;
    }

    case XK_Tab: {
        send_key('\t');
        return;
    }

    case XK_Delete: {
        send_key(0x7f);
        return;
    }
    case XK_BackSpace: {
        send_key('\010');
        return;
    }
    }

    if (ks) {
        sprintf(msg, "%c%s%s%s%s_%lX%c", 31, mc ? "N" : "", ms ? "S" : "", mo ? "O" : "", mx ? "M" : "", (unsigned long)(ks), 13);
    } else {
        sprintf(msg, "%c%s%s%s%sK_%X%c", 31, mc ? "N" : "", ms ? "S" : "", mo ? "O" : "", mx ? "M" : "", ev->keycode, 13);
    }

    send_keys(msg);

    if (n && (macro_find_exact(msg) < 0)) {
        macro_add(msg, buf);
    }
}

/*
 * Find the square a particular pixel is part of.
 */
static void pixel_to_square(int *const x, int *const y, const int ox, const int oy)
{
    (*x) = (ox - Infowin->ox) / Infofnt->wid;
    (*y) = (oy - Infowin->oy) / Infofnt->hgt;
}

/*
 * Find the pixel at the top-left corner of a square.
 */
static void square_to_pixel(int *const x, int *const y, const int ox, const int oy)
{
    (*x) = ox * Infofnt->wid + Infowin->ox;
    (*y) = oy * Infofnt->hgt + Infowin->oy;
}

/*
 * Convert co-ordinates from starting corner/opposite corner to minimum/maximum.
 */
static void sort_co_ord(co_ord *min, co_ord *max, const co_ord *b, const co_ord *a)
{
    min->x = MIN(a->x, b->x);
    min->y = MIN(a->y, b->y);
    max->x = MAX(a->x, b->x);
    max->y = MAX(a->y, b->y);
}

/*
 * Remove the selection by redrawing it.
 */
static void mark_selection_clear(int x1, int y1, int x2, int y2)
{
    term_redraw_section(x1, y1, x2, y2);
}

/*
 * Select an area by drawing a grey box around it.
 * NB. These two functions can cause flicker as the selection is modified,
 * as the game redraws the entire marked section.
 */
static void mark_selection_mark(int x1, int y1, int x2, int y2)
{
    square_to_pixel(&x1, &y1, x1, y1);
    square_to_pixel(&x2, &y2, x2, y2);
#ifdef USE_XFT
    XftDrawRect(Infowin->draw, &clr[2]->fg, x1, y1, x2 - x1 + Infofnt->wid - 1, y2 - y1 + Infofnt->hgt - 1);
#else
    XDrawRectangle(Metadpy->dpy, Infowin->win, clr[2]->gc, x1, y1, x2 - x1 + Infofnt->wid - 1, y2 - y1 + Infofnt->hgt - 1);
#endif
}

/*
 * Mark a selection by drawing boxes around it (for now).
 */
static void mark_selection(void)
{
    co_ord min, max;
    term_type *old = Term;
    bool draw = s_ptr->select;
    bool clear = s_ptr->drawn;
    if (s_ptr->t != old)
        term_activate(s_ptr->t);

    if (clear) {
        sort_co_ord(&min, &max, &s_ptr->init, &s_ptr->old);
        mark_selection_clear(min.x, min.y, max.x, max.y);
    }
    if (draw) {
        sort_co_ord(&min, &max, &s_ptr->init, &s_ptr->cur);
        mark_selection_mark(min.x, min.y, max.x, max.y);
    }

    if (s_ptr->t != old)
        term_activate(old);

    s_ptr->old.x = s_ptr->cur.x;
    s_ptr->old.y = s_ptr->cur.y;
    s_ptr->drawn = s_ptr->select;
}

/*
 * Forget a selection for one reason or another.
 */
static void copy_x11_release(void)
{
    s_ptr->select = false;
    mark_selection();
}

/*
 * Start to select some text on the screen.
 */
static void copy_x11_start(int x, int y)
{
    if (s_ptr->select)
        copy_x11_release();

    s_ptr->t = Term;
    s_ptr->init.x = s_ptr->cur.x = s_ptr->old.x = x;
    s_ptr->init.y = s_ptr->cur.y = s_ptr->old.y = y;
}

/*
 * Respond to movement of the mouse when selecting text.
 */
static void copy_x11_cont(int x, int y, unsigned int buttons)
{
    x = MIN(MAX(x, 0), Term->wid - 1);
    y = MIN(MAX(y, 0), Term->hgt - 1);
    if (~buttons & Button1Mask)
        return;
    if (s_ptr->t != Term)
        return;
    if (x == s_ptr->old.x && y == s_ptr->old.y && s_ptr->select)
        return;

    s_ptr->select = true;
    s_ptr->cur.x = x;
    s_ptr->cur.y = y;
    mark_selection();
}

/*
 * Respond to release of the left mouse button by putting the selected text in
 * the primary buffer.
 */
static void copy_x11_end(const Time time)
{
    if (!s_ptr->select)
        return;
    if (s_ptr->t != Term)
        return;

    s_ptr->time = time;
    XSetSelectionOwner(Metadpy->dpy, XA_PRIMARY, Infowin->win, time);
    if (XGetSelectionOwner(Metadpy->dpy, XA_PRIMARY) != Infowin->win) {
        s_ptr->select = false;
        mark_selection();
    }
}

static Atom xa_targets, xa_timestamp, xa_text, xa_compound_text, xa_utf8;

/*
 * Set the required variable atoms at start-up to avoid errors later.
 */
static void set_atoms(void)
{
    xa_targets = XInternAtom(DPY, "TARGETS", False);
    xa_timestamp = XInternAtom(DPY, "TIMESTAMP", False);
    xa_text = XInternAtom(DPY, "TEXT", False);
    xa_compound_text = XInternAtom(DPY, "COMPOUND_TEXT", False);
    xa_utf8 = XInternAtom(DPY, "UTF8_STRING", False);
}

static Atom request_target = 0;

/*
 * Send a message to request that the PRIMARY buffer be sent here.
 */
static void paste_x11_request(Atom target, const Time time)
{
    Atom property = XInternAtom(DPY, "__COPY_TEXT", False);
    if (XGetSelectionOwner(DPY, XA_PRIMARY) == None) {
        /* No selection. */
        /* bell("No selection found."); */
        return;
    }

    request_target = target;
    XConvertSelection(DPY, XA_PRIMARY, target, property, WIN, time);
}

/*
 * Add the contents of the PRIMARY buffer to the input queue.
 *
 * Hack - This doesn't use the "time" of the event, and so accepts anything a
 * client tries to send it.
 */
static void paste_x11_accept(const XSelectionEvent *ptr)
{
    unsigned long left;
    const long offset = 0;
    const long length = 32000;
    XTextProperty xtextproperty;
    errr err = 0;
    Atom property = XInternAtom(DPY, "__COPY_TEXT", False);
    if (ptr->property == None) {
        if (request_target == xa_compound_text) {
            paste_x11_request(XA_STRING, ptr->time);
        } else {
            request_target = 0;
            plog("Paste failure (remote client could not send).");
        }

        return;
    }

    if (ptr->selection != XA_PRIMARY) {
        plog("Paste failure (remote client did not send primary selection).");
        return;
    }

    if (ptr->target != request_target) {
        plog("Paste failure (selection in unknown format).");
        return;
    }

    if (XGetWindowProperty(Metadpy->dpy, Infowin->win, property, offset, length, true, request_target, &xtextproperty.encoding, &xtextproperty.format,
            &xtextproperty.nitems, &left, &xtextproperty.value)
        != Success) {
        return;
    }

    if (request_target == xa_compound_text) {
        char **list;
        int count;

        XmbTextPropertyToTextList(DPY, &xtextproperty, &list, &count);

        if (list) {
            int i;

            for (i = 0; i < count; i++) {
                err = type_string(list[i], 0);
                if (err)
                    break;
            }

            XFreeStringList(list);
        }
    } else {
        err = type_string((char *)xtextproperty.value, xtextproperty.nitems);
    }

    XFree(xtextproperty.value);
    if (err) {
        plog("Paste failure (too much text selected).");
    }
}

/*
 * Add a character to a string in preparation for sending it to another
 * client as a STRING.
 * This doesn't change anything, as clients tend not to have difficulty in
 * receiving this format (although the standard specifies a restricted set).
 * Strings do not have a colour.
 */
static bool paste_x11_send_text(XSelectionRequestEvent *rq)
{
    char buf[1024];
    co_ord max, min;
    TERM_LEN x, y;
    int l;
    TERM_COLOR a;
    char c;

    sort_co_ord(&min, &max, &s_ptr->init, &s_ptr->cur);
    if (XGetSelectionOwner(DPY, XA_PRIMARY) != WIN) {
        return false;
    }

    for (y = 0; y < Term->hgt; y++) {
#ifdef JP
        int kanji = 0;
#endif
        if (y < min.y)
            continue;
        if (y > max.y)
            break;

        for (l = 0, x = 0; x < Term->wid; x++) {
#ifdef JP
            if (x > max.x)
                break;

            term_what(x, y, &a, &c);
            if (1 == kanji)
                kanji = 2;
            else if (iskanji(c))
                kanji = 1;
            else
                kanji = 0;

            if (x < min.x)
                continue;

            /*
             * A single kanji character was divided in two...
             * Delete the garbage.
             */
            if ((2 == kanji && x == min.x) || (1 == kanji && x == max.x))
                c = ' ';
#else
            if (x > max.x)
                break;
            if (x < min.x)
                continue;

            term_what(x, y, &a, &c);
#endif

            buf[l] = c;
            l++;
        }

        // 末尾の空白を削る。
        while (l >= 1 && buf[l - 1] == ' ')
            l--;

        // 複数行の場合、各行末に改行を付加。
        if (min.y != max.y) {
            buf[l] = '\n';
            l++;
        }

        buf[l] = '\0';

#ifdef JP
        char utf8_buf[2048];
        const int len = euc_to_utf8(buf, l, utf8_buf, sizeof(utf8_buf));
#endif
        if (_(len, l) > 0) {
            XChangeProperty(DPY, rq->requestor, rq->property, xa_utf8, 8, PropModeAppend, (unsigned char *)_(utf8_buf, buf), _(len, l));
        }
    }

    return true;
}

/*
 * Send some text requested by another X client.
 */
static void paste_x11_send(XSelectionRequestEvent *rq)
{
    XEvent event;
    XSelectionEvent *ptr = &(event.xselection);

    ptr->type = SelectionNotify;
    ptr->property = rq->property;
    ptr->display = rq->display;
    ptr->requestor = rq->requestor;
    ptr->selection = rq->selection;
    ptr->target = rq->target;
    ptr->time = rq->time;

    /*
     * Paste the appropriate information for each target type.
     * Note that this currently rejects MULTIPLE targets.
     */

    if (rq->target == xa_utf8) {
        if (!paste_x11_send_text(rq))
            ptr->property = None;
    } else if (rq->target == xa_targets) {
        Atom target_list[] = { xa_targets, xa_utf8 };
        XChangeProperty(
            DPY, rq->requestor, rq->property, XA_ATOM, 32, PropModeReplace, (unsigned char *)target_list, (sizeof(target_list) / sizeof(target_list[0])));
    } else {
        ptr->property = None;
    }

    XSendEvent(DPY, rq->requestor, false, NoEventMask, &event);
}

/*
 * Handle various events conditional on presses of a mouse button.
 */
static void handle_button(Time time, int x, int y, int button, bool press)
{
    pixel_to_square(&x, &y, x, y);

    if (press && button == 1)
        copy_x11_start(x, y);
    if (!press && button == 1)
        copy_x11_end(time);
    if (!press && button == 2)
        paste_x11_request(xa_compound_text, time);
}

/*
 * Process events
 */
static errr CheckEvent(bool wait)
{
    term_data *old_td = (term_data *)(Term->data);

    XEvent xev_body, *xev = &xev_body;

    term_data *td = nullptr;
    infowin *iwin = nullptr;

    int i;

#ifdef USE_XIM
    do {
#endif

        if (!wait && !XPending(Metadpy->dpy))
            return (1);

        if (s_ptr->select && !s_ptr->drawn)
            mark_selection();

        XNextEvent(Metadpy->dpy, xev);

#ifdef USE_XIM
    } while (XFilterEvent(xev, xev->xany.window));
#endif

    if (xev->type == MappingNotify) {
        XRefreshKeyboardMapping(&xev->xmapping);
        return 0;
    }

    for (i = 0; i < MAX_TERM_DATA; i++) {
        if (!data[i].win)
            continue;
        if (xev->xany.window == data[i].win->win) {
            td = &data[i];
            iwin = td->win;
            break;
        }
    }

    if (!td || !iwin)
        return (0);

    term_activate(&td->t);
    Infowin_set(iwin);
    switch (xev->type) {
    case ButtonPress:
    case ButtonRelease: {
        bool press = (xev->type == ButtonPress);
        int x = xev->xbutton.x;
        int y = xev->xbutton.y;
        int z;
        if (xev->xbutton.button == Button1)
            z = 1;
        else if (xev->xbutton.button == Button2)
            z = 2;
        else if (xev->xbutton.button == Button3)
            z = 3;
        else if (xev->xbutton.button == Button4)
            z = 4;
        else if (xev->xbutton.button == Button5)
            z = 5;
        else
            z = 0;

        handle_button(xev->xbutton.time, x, y, z, press);
        break;
    }
    case EnterNotify:
    case LeaveNotify: {
        break;
    }
    case MotionNotify: {
        int x = xev->xmotion.x;
        int y = xev->xmotion.y;
        unsigned int z = xev->xmotion.state;
        pixel_to_square(&x, &y, x, y);
        copy_x11_cont(x, y, z);
        break;
    }
    case SelectionNotify: {
        paste_x11_accept(&(xev->xselection));
        break;
    }
    case SelectionRequest: {
        paste_x11_send(&(xev->xselectionrequest));
        break;
    }
    case SelectionClear: {
        s_ptr->select = false;
        mark_selection();
        break;
    }
    case KeyRelease: {
        break;
    }
    case KeyPress: {
        term_activate(&old_td->t);
        react_keypress(&(xev->xkey));
        break;
    }
    case Expose: {
        int x1, x2, y1, y2;
        x1 = (xev->xexpose.x - Infowin->ox) / Infofnt->wid;
        x2 = (xev->xexpose.x + xev->xexpose.width - Infowin->ox) / Infofnt->wid;

        y1 = (xev->xexpose.y - Infowin->oy) / Infofnt->hgt;
        y2 = (xev->xexpose.y + xev->xexpose.height - Infowin->oy) / Infofnt->hgt;

        term_redraw_section(x1, y1, x2, y2);
        break;
    }
    case MapNotify: {
        Infowin->mapped = 1;
        Term->mapped_flag = true;
        break;
    }
    case UnmapNotify: {
        Infowin->mapped = 0;
        Term->mapped_flag = false;
        break;
    }
    case ConfigureNotify: {
        int cols, rows, wid, hgt;
        int ox = Infowin->ox;
        int oy = Infowin->oy;
        Infowin->x = xev->xconfigure.x;
        Infowin->y = xev->xconfigure.y;
        Infowin->w = xev->xconfigure.width;
        Infowin->h = xev->xconfigure.height;
        cols = ((Infowin->w - (ox + ox)) / td->fnt->wid);
        rows = ((Infowin->h - (oy + oy)) / td->fnt->hgt);
        if (cols < 1)
            cols = 1;
        if (rows < 1)
            rows = 1;

        if (td == &data[0]) {
            if (cols < 80)
                cols = 80;
            if (rows < 24)
                rows = 24;
        }

        wid = cols * td->fnt->wid + (ox + ox);
        hgt = rows * td->fnt->hgt + (oy + oy);
        term_resize(cols, rows);
        if ((Infowin->w != wid) || (Infowin->h != hgt)) {
            Infowin_set(td->win);
            Infowin_resize(wid, hgt);
        }

        break;
    }
#ifdef USE_XIM
    case FocusIn: {
        if (iwin->xic) {
            XSetICFocus(iwin->xic);
        }
        Focuswin = iwin;
        break;
    }
    case FocusOut: {
        if (iwin->xic) {
            XUnsetICFocus(iwin->xic);
        }

        break;
    }
#endif
    }

    term_activate(&old_td->t);
    Infowin_set(old_td->win);
    return (0);
}

/*
 * An array of sound file names
 */
static concptr sound_file[SOUND_MAX];

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
static void init_sound(void)
{
    int i;
    char wav[128];
    char buf[1024];
    char dir_xtra_sound[1024];
    path_build(dir_xtra_sound, sizeof(dir_xtra_sound), ANGBAND_DIR_XTRA, "sound");
    for (i = 1; i < SOUND_MAX; i++) {
        sprintf(wav, "%s.wav", angband_sound_name[i]);
        path_build(buf, sizeof(buf), dir_xtra_sound, wav);
        if (check_file(buf))
            sound_file[i] = string_make(buf);
    }

    use_sound = true;
    return;
}

/*
 * Hack -- make a sound
 */
static errr Term_xtra_x11_sound(int v)
{
    char buf[1024];
    if (!use_sound)
        return (1);
    if ((v < 0) || (v >= SOUND_MAX))
        return (1);
    if (!sound_file[v])
        return (1);

    sprintf(buf, "./playwave.sh %s\n", sound_file[v]);
    return (system(buf) < 0);
}

/*
 * Handle "activation" of a term
 */
static errr Term_xtra_x11_level(int v)
{
    term_data *td = (term_data *)(Term->data);
    if (v) {
        Infowin_set(td->win);
        Infofnt_set(td->fnt);
    }

    return (0);
}

/*
 * React to changes
 */
static errr Term_xtra_x11_react(void)
{
    int i;

    if (Metadpy->color) {
        for (i = 0; i < 256; i++) {
            if ((color_table[i][0] != angband_color_table[i][0]) || (color_table[i][1] != angband_color_table[i][1])
                || (color_table[i][2] != angband_color_table[i][2]) || (color_table[i][3] != angband_color_table[i][3])) {
                Pixell pixel;
                color_table[i][0] = angband_color_table[i][0];
                color_table[i][1] = angband_color_table[i][1];
                color_table[i][2] = angband_color_table[i][2];
                color_table[i][3] = angband_color_table[i][3];
                pixel = create_pixel(Metadpy->dpy, color_table[i][1], color_table[i][2], color_table[i][3]);
                Infoclr_set(clr[i]);
                Infoclr_change_fg(pixel);
            }
        }
    }

    return (0);
}

/*
 * Handle a "special request"
 */
static errr Term_xtra_x11(int n, int v)
{
    switch (n) {
    case TERM_XTRA_NOISE:
        Metadpy_do_beep();
        return (0);
    case TERM_XTRA_SOUND:
        return (Term_xtra_x11_sound(v));
#ifdef USE_XFT
    case TERM_XTRA_FRESH:
        Metadpy_update(1, 1, 0);
        return (0);
#else
    case TERM_XTRA_FRESH:
        Metadpy_update(1, 0, 0);
        return (0);
#endif
    case TERM_XTRA_BORED:
        return (CheckEvent(0));
    case TERM_XTRA_EVENT:
        return (CheckEvent(v));
    case TERM_XTRA_FLUSH:
        while (!CheckEvent(false))
            ;
        return (0);
    case TERM_XTRA_LEVEL:
        return (Term_xtra_x11_level(v));
    case TERM_XTRA_CLEAR:
        Infowin_wipe();
        s_ptr->drawn = false;
        return (0);
    case TERM_XTRA_DELAY:
        usleep(1000 * v);
        return (0);
    case TERM_XTRA_REACT:
        return (Term_xtra_x11_react());
    }

    return (1);
}

/*
 * Draw the cursor as an inverted rectangle.
 *
 * Consider a rectangular outline like "main-mac.c".  XXX XXX
 */
static errr Term_curs_x11(int x, int y)
{
    if (use_graphics) {
#ifdef USE_XFT
        XftDrawRect(Infowin->draw, &xor_->fg, x * Infofnt->wid + Infowin->ox, y * Infofnt->hgt + Infowin->oy, Infofnt->wid - 1, Infofnt->hgt - 1);
        XftDrawRect(Infowin->draw, &xor_->fg, x * Infofnt->wid + Infowin->ox + 1, y * Infofnt->hgt + Infowin->oy + 1, Infofnt->wid - 3, Infofnt->hgt - 3);
#else
        XDrawRectangle(
            Metadpy->dpy, Infowin->win, xor_->gc, x * Infofnt->wid + Infowin->ox, y * Infofnt->hgt + Infowin->oy, Infofnt->wid - 1, Infofnt->hgt - 1);
        XDrawRectangle(
            Metadpy->dpy, Infowin->win, xor_->gc, x * Infofnt->wid + Infowin->ox + 1, y * Infofnt->hgt + Infowin->oy + 1, Infofnt->wid - 3, Infofnt->hgt - 3);
#endif
    } else {
        Infoclr_set(xor_);
        Infofnt_text_non(x, y, " ", 1);
    }

    return (0);
}

/*
 * Draw the double width cursor
 */
static errr Term_bigcurs_x11(int x, int y)
{
    if (use_graphics) {
#ifdef USE_XFT
        XftDrawRect(Infowin->draw, &xor_->fg, x * Infofnt->wid + Infowin->ox, y * Infofnt->hgt + Infowin->oy, Infofnt->twid - 1, Infofnt->hgt - 1);
        XftDrawRect(Infowin->draw, &xor_->fg, x * Infofnt->wid + Infowin->ox + 1, y * Infofnt->hgt + Infowin->oy + 1, Infofnt->twid - 3, Infofnt->hgt - 3);
#else
        XDrawRectangle(
            Metadpy->dpy, Infowin->win, xor_->gc, x * Infofnt->wid + Infowin->ox, y * Infofnt->hgt + Infowin->oy, Infofnt->twid - 1, Infofnt->hgt - 1);
        XDrawRectangle(
            Metadpy->dpy, Infowin->win, xor_->gc, x * Infofnt->wid + Infowin->ox + 1, y * Infofnt->hgt + Infowin->oy + 1, Infofnt->twid - 3, Infofnt->hgt - 3);
#endif
    } else {
        Infoclr_set(xor_);
        Infofnt_text_non(x, y, "  ", 2);
    }

    return (0);
}

/*
 * Erase some characters.
 */
static errr Term_wipe_x11(int x, int y, int n)
{
    Infoclr_set(clr[TERM_DARK]);
    Infofnt_text_non(x, y, "", n);
    s_ptr->drawn = false;
    return (0);
}

/*
 * Draw some textual characters.
 */
static errr Term_text_x11(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr s)
{
    Infoclr_set(clr[a]);
    Infofnt_text_std(x, y, s, n);
    s_ptr->drawn = false;
    return (0);
}

#ifndef USE_XFT
/*
 * Draw some graphical characters.
 */
static errr Term_pict_x11(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, const char *cp, const TERM_COLOR *tap, const char *tcp)
{
    int i, x1, y1;

    TERM_COLOR a;
    char c;

    TERM_COLOR ta;
    char tc;

    int x2, y2;
    int k, l;

    unsigned long pixel, blank;

    term_data *td = (term_data *)(Term->data);

    y *= Infofnt->hgt;
    x *= Infofnt->wid;

    y += Infowin->oy;
    x += Infowin->ox;
    for (i = 0; i < n; ++i, x += td->fnt->wid) {
        a = *ap++;
        c = *cp++;
        x1 = (c & 0x7F) * td->fnt->twid;
        y1 = (a & 0x7F) * td->fnt->hgt;
        if (td->tiles->width < x1 + td->fnt->wid || td->tiles->height < y1 + td->fnt->hgt) {
            XFillRectangle(Metadpy->dpy, td->win->win, clr[0]->gc, x, y, td->fnt->twid, td->fnt->hgt);
            continue;
        }

        ta = *tap++;
        tc = *tcp++;

        x2 = (tc & 0x7F) * td->fnt->twid;
        y2 = (ta & 0x7F) * td->fnt->hgt;

        if (((x1 == x2) && (y1 == y2)) || !(((byte)ta & 0x80) && ((byte)tc & 0x80)) || td->tiles->width < x2 + td->fnt->wid
            || td->tiles->height < y2 + td->fnt->hgt) {
            XPutImage(Metadpy->dpy, td->win->win, clr[0]->gc, td->tiles, x1, y1, x, y, td->fnt->twid, td->fnt->hgt);
        } else {
            blank = XGetPixel(td->tiles, 0, td->fnt->hgt * 6);
            for (k = 0; k < td->fnt->twid; k++) {
                for (l = 0; l < td->fnt->hgt; l++) {
                    if ((pixel = XGetPixel(td->tiles, x1 + k, y1 + l)) == blank) {
                        pixel = XGetPixel(td->tiles, x2 + k, y2 + l);
                    }

                    XPutPixel(td->TmpImage, k, l, pixel);
                }
            }

            XPutImage(Metadpy->dpy, td->win->win, clr[0]->gc, td->TmpImage, 0, 0, x, y, td->fnt->twid, td->fnt->hgt);
        }
    }

    s_ptr->drawn = false;
    return (0);
}
#endif

#ifdef USE_XIM
static void IMDestroyCallback(XIM, XPointer, XPointer);

static void IMInstantiateCallback(Display *display, XPointer unused1, XPointer unused2)
{
    XIM xim;
    XIMCallback ximcallback;
    XIMStyles *xim_styles = nullptr;
    int i;

    (void)unused1;
    (void)unused2;

    xim = XOpenIM(display, nullptr, nullptr, nullptr);
    if (!xim) {
        printf("can't open IM\n");
        return;
    }

    ximcallback.callback = IMDestroyCallback;
    ximcallback.client_data = nullptr;
    XSetIMValues(xim, XNDestroyCallback, &ximcallback, nullptr);
    XGetIMValues(xim, XNQueryInputStyle, &xim_styles, nullptr);
    for (i = 0; i < xim_styles->count_styles; i++) {
        if (xim_styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing))
            break;
    }
    if (i >= xim_styles->count_styles) {
        printf("Sorry, your IM does not support 'Root' preedit style...\n");
        XCloseIM(xim);
        return;
    }
    XFree(xim_styles);

    Metadpy->xim = xim;

    for (i = 0; i < MAX_TERM_DATA; i++) {
        infowin *iwin = data[i].win;
        if (!iwin)
            continue;
        iwin->xic = XCreateIC(xim, XNInputStyle, (XIMPreeditNothing | XIMStatusNothing), XNClientWindow, iwin->win, XNFocusWindow, iwin->win, nullptr);
        if (!iwin->xic) {
            printf("Can't create input context for Term%d\n", i);
            continue;
        }

        if (XGetICValues(iwin->xic, XNFilterEvents, &iwin->xic_mask, nullptr) != nullptr) {
            iwin->xic_mask = 0L;
        }

        XSelectInput(Metadpy->dpy, iwin->win, iwin->mask | iwin->xic_mask);
    }

    return;
}

static void IMDestroyCallback(XIM xim, XPointer client_data, XPointer call_data)
{
    int i;
    (void)xim;
    (void)client_data;

    if (call_data == nullptr) {
        XRegisterIMInstantiateCallback(Metadpy->dpy, nullptr, nullptr, nullptr, IMInstantiateCallback, nullptr);
    }

    for (i = 0; i < MAX_TERM_DATA; i++) {
        infowin *iwin = data[i].win;
        if (!iwin)
            continue;
        if (iwin->xic_mask) {
            XSelectInput(Metadpy->dpy, iwin->win, iwin->mask);
            iwin->xic_mask = 0L;
        }
        iwin->xic = nullptr;
    }

    Metadpy->xim = nullptr;
}
#endif

static char force_lower(char a)
{
    return ((isupper((a))) ? tolower((a)) : (a));
}

static void Term_nuke_x11(term_type *)
{
    for (auto i = 0; i < MAX_TERM_DATA; i++) {
        infofnt *ifnt = data[i].fnt;
        infowin *iwin = data[i].win;
        if (ifnt && ifnt->info)
#ifdef USE_XFT
            XftFontClose(Metadpy->dpy, ifnt->info);
#else
            XFreeFontSet(Metadpy->dpy, ifnt->info);
#endif
        if (iwin && iwin->xic)
            XDestroyIC(iwin->xic);
#ifdef USE_XFT
        if (iwin && iwin->draw)
            XftDrawDestroy(iwin->draw);
#endif
        angband_term[i] = nullptr;
    }

    if (Metadpy->xim)
        XCloseIM(Metadpy->xim);
    XUnregisterIMInstantiateCallback(Metadpy->dpy, NULL, NULL, NULL, IMInstantiateCallback, NULL);
    XCloseDisplay(Metadpy->dpy);
}

/*
 * Initialize a term_data
 */
static errr term_data_init(term_data *td, int i)
{
    term_type *t = &td->t;

    concptr name = angband_term_name[i];

    concptr font;
    int x = 0;
    int y = 0;

    int cols = 80;
    int rows = 24;

    int ox = 1;
    int oy = 1;

    int wid, hgt, num;

    char buf[80];

    concptr str;

    int val;

    XClassHint *ch;

    char res_name[20];
    char res_class[20];

    XSizeHints *sh;
#ifdef USE_XIM
    XWMHints *wh;
#endif

    sprintf(buf, "ANGBAND_X11_FONT_%d", i);
    font = getenv(buf);
    if (!font)
        font = getenv("ANGBAND_X11_FONT");

    if (!font) {
        switch (i) {
        case 0: {
            font = DEFAULT_X11_FONT_0;
        } break;
        case 1: {
            font = DEFAULT_X11_FONT_1;
        } break;
        case 2: {
            font = DEFAULT_X11_FONT_2;
        } break;
        case 3: {
            font = DEFAULT_X11_FONT_3;
        } break;
        case 4: {
            font = DEFAULT_X11_FONT_4;
        } break;
        case 5: {
            font = DEFAULT_X11_FONT_5;
        } break;
        case 6: {
            font = DEFAULT_X11_FONT_6;
        } break;
        case 7: {
            font = DEFAULT_X11_FONT_7;
        } break;
        default: {
            font = DEFAULT_X11_FONT;
        }
        }
    }

    sprintf(buf, "ANGBAND_X11_AT_X_%d", i);
    str = getenv(buf);
    x = (str != nullptr) ? atoi(str) : -1;

    sprintf(buf, "ANGBAND_X11_AT_Y_%d", i);
    str = getenv(buf);
    y = (str != nullptr) ? atoi(str) : -1;

    sprintf(buf, "ANGBAND_X11_COLS_%d", i);
    str = getenv(buf);
    val = (str != nullptr) ? atoi(str) : -1;
    if (val > 0)
        cols = val;

    sprintf(buf, "ANGBAND_X11_ROWS_%d", i);
    str = getenv(buf);
    val = (str != nullptr) ? atoi(str) : -1;
    if (val > 0)
        rows = val;

    if (!i) {
        if (cols < 80)
            cols = 80;
        if (rows < 24)
            rows = 24;
    }

    sprintf(buf, "ANGBAND_X11_IBOX_%d", i);
    str = getenv(buf);
    val = (str != nullptr) ? atoi(str) : -1;
    if (val > 0)
        ox = val;

    sprintf(buf, "ANGBAND_X11_IBOY_%d", i);
    str = getenv(buf);
    val = (str != nullptr) ? atoi(str) : -1;
    if (val > 0)
        oy = val;

    MAKE(td->fnt, infofnt);
    Infofnt_set(td->fnt);
    Infofnt_init_data(font);

    num = ((i == 0) ? 1024 : 16);
    wid = cols * td->fnt->wid + (ox + ox);
    hgt = rows * td->fnt->hgt + (oy + oy);
    MAKE(td->win, infowin);
    Infowin_set(td->win);
    Infowin_init_top(x, y, wid, hgt, 0, Metadpy->fg, Metadpy->bg);

#if defined(USE_XIM)
    Infowin_set_mask(ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask);
#else
    Infowin_set_mask(ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
#endif

    Infowin_set_name(name);
    Infowin->ox = ox;
    Infowin->oy = oy;
    ch = XAllocClassHint();

    if (ch == nullptr)
        quit("XAllocClassHint failed");

    strcpy(res_name, name);
    res_name[0] = force_lower(res_name[0]);
    ch->res_name = res_name;

    strcpy(res_class, "Angband");
    ch->res_class = res_class;

    XSetClassHint(Metadpy->dpy, Infowin->win, ch);
    XFree(ch);
    sh = XAllocSizeHints();
    if (sh == nullptr)
        quit("XAllocSizeHints failed");

    if (i == 0) {
        sh->flags = PMinSize | PMaxSize;
        sh->min_width = 80 * td->fnt->wid + (ox + ox);
        sh->min_height = 24 * td->fnt->hgt + (oy + oy);
        sh->max_width = 255 * td->fnt->wid + (ox + ox);
        sh->max_height = 255 * td->fnt->hgt + (oy + oy);
    } else {
        sh->flags = PMinSize | PMaxSize;
        sh->min_width = td->fnt->wid + (ox + ox);
        sh->min_height = td->fnt->hgt + (oy + oy);
        sh->max_width = 256 * td->fnt->wid + (ox + ox);
        sh->max_height = 256 * td->fnt->hgt + (oy + oy);
    }

    sh->flags |= PResizeInc;
    sh->width_inc = td->fnt->wid;
    sh->height_inc = td->fnt->hgt;
    sh->flags |= PBaseSize;
    sh->base_width = (ox + ox);
    sh->base_height = (oy + oy);
    XSetWMNormalHints(Metadpy->dpy, Infowin->win, sh);
    XFree(sh);
    Infowin_map();

#ifdef USE_XIM
    wh = XAllocWMHints();
    if (wh == nullptr)
        quit("XAllocWMHints failed");
    wh->flags = InputHint;
    wh->input = True;
    XSetWMHints(Metadpy->dpy, Infowin->win, wh);
    XFree(wh);
#endif

    if ((x >= 0) && (y >= 0))
        Infowin_impell(x, y);

    term_init(t, cols, rows, num);
    t->soft_cursor = true;
    t->attr_blank = TERM_WHITE;
    t->char_blank = ' ';
    t->xtra_hook = Term_xtra_x11;
    t->curs_hook = Term_curs_x11;
    t->bigcurs_hook = Term_bigcurs_x11;
    t->wipe_hook = Term_wipe_x11;
    t->text_hook = Term_text_x11;
    t->nuke_hook = Term_nuke_x11;
    t->data = td;
    term_activate(t);
    return (0);
}

/*
 * Initialization function for an "X11" module to Angband
 */
errr init_x11(int argc, char *argv[])
{
    int i;
    concptr dpy_name = "";
    int num_term = 3;

#ifndef USE_XFT
    char filename[1024];

    int pict_wid = 0;
    int pict_hgt = 0;

    char *TmpData;
#endif

    for (i = 1; i < argc; i++) {
        if (prefix(argv[i], "-d")) {
            dpy_name = &argv[i][2];
            continue;
        }

#ifndef USE_XFT
        if (prefix(argv[i], "-s")) {
            smoothRescaling = false;
            continue;
        }

        if (prefix(argv[i], "-a")) {
            arg_graphics = GRAPHICS_ADAM_BOLT;
            continue;
        }

        if (prefix(argv[i], "-o")) {
            arg_graphics = GRAPHICS_ORIGINAL;
            continue;
        }
#endif

        if (prefix(argv[i], "-b")) {
            arg_bigtile = use_bigtile = true;
            continue;
        }

        if (prefix(argv[i], "-n")) {
            num_term = atoi(&argv[i][2]);
            if (num_term > MAX_TERM_DATA)
                num_term = MAX_TERM_DATA;
            else if (num_term < 1)
                num_term = 1;
            continue;
        }

        if (prefix(argv[i], "--")) {
            continue;
        }

        plog_fmt("Ignoring option: %s", argv[i]);
    }

#ifdef USE_LOCALE

#ifdef JP
    setlocale(LC_ALL, "");

#ifdef DEFAULT_LOCALE
    if (!strcmp(setlocale(LC_ALL, nullptr), "C")) {
        printf("try default locale \"%s\"\n", DEFAULT_LOCALE);
        setlocale(LC_ALL, DEFAULT_LOCALE);
    }
#endif

    if (!strcmp(setlocale(LC_ALL, nullptr), "C")) {
        printf("WARNING: Locale is not supported. Non-english font may be displayed incorrectly.\n");
    }

    if (!XSupportsLocale()) {
        printf("can't support locale in X\n");
        setlocale(LC_ALL, "C");
    }
#else
    setlocale(LC_ALL, "C");
#endif /* JP */

#endif /* USE_LOCALE */

    if (Metadpy_init_name(dpy_name))
        return (-1);

    MAKE(xor_, infoclr);
    Infoclr_set(xor_);
    Infoclr_init_ppn(Metadpy->fg, Metadpy->bg, "xor", 0);
    for (i = 0; i < 256; ++i) {
        Pixell pixel;
        MAKE(clr[i], infoclr);
        Infoclr_set(clr[i]);
        color_table[i][0] = angband_color_table[i][0];
        color_table[i][1] = angband_color_table[i][1];
        color_table[i][2] = angband_color_table[i][2];
        color_table[i][3] = angband_color_table[i][3];
        pixel = ((i == 0) ? Metadpy->bg : Metadpy->fg);
        if (Metadpy->color) {
            pixel = create_pixel(Metadpy->dpy, color_table[i][1], color_table[i][2], color_table[i][3]);
        }

        Infoclr_init_ppn(pixel, Metadpy->bg, "cpy", 0);
    }

    set_atoms();
    for (i = 0; i < num_term; i++) {
        term_data *td = &data[i];
        term_data_init(td, i);
        angband_term[i] = Term;
    }

    Infowin_set(data[0].win);
    Infowin_raise();
    term_activate(&data[0].t);

#ifdef USE_XIM
    {
        char *p;
        p = XSetLocaleModifiers("");
        if (!p || !*p) {
            p = XSetLocaleModifiers("@im=");
        }
    }
    XRegisterIMInstantiateCallback(Metadpy->dpy, nullptr, nullptr, nullptr, IMInstantiateCallback, nullptr);
#endif

    if (arg_sound)
        init_sound();

#ifndef USE_XFT
    switch (arg_graphics) {
    case GRAPHICS_ORIGINAL:
        path_build(filename, sizeof(filename), ANGBAND_DIR_XTRA, "graf/8x8.bmp");
        if (0 == fd_close(fd_open(filename, O_RDONLY))) {
            use_graphics = true;
            pict_wid = pict_hgt = 8;
            ANGBAND_GRAF = "old";
        }
        break;
    case GRAPHICS_ADAM_BOLT:
        path_build(filename, sizeof(filename), ANGBAND_DIR_XTRA, "graf/16x16.bmp");
        if (0 == fd_close(fd_open(filename, O_RDONLY))) {
            use_graphics = true;
            pict_wid = pict_hgt = 16;
            ANGBAND_GRAF = "new";
        }
        break;
    }

    if (use_graphics) {
        Display *dpy = Metadpy->dpy;
        XImage *tiles_raw;
        tiles_raw = ReadBMP(dpy, filename);
        for (i = 0; i < num_term; i++) {
            term_data *td = &data[i];
            term_type *t = &td->t;
            t->pict_hook = Term_pict_x11;
            t->higher_pict = true;
            td->tiles = ResizeImage(dpy, tiles_raw, pict_wid, pict_hgt, td->fnt->twid, td->fnt->hgt);
        }

        for (i = 0; i < num_term; i++) {
            term_data *td = &data[i];
            int ii, jj;
            int depth = DefaultDepth(dpy, DefaultScreen(dpy));
            Visual *visual = DefaultVisual(dpy, DefaultScreen(dpy));
            int total;
            ii = 1;
            jj = (depth - 1) >> 2;
            while (jj >>= 1)
                ii <<= 1;
            total = td->fnt->twid * td->fnt->hgt * ii;
            TmpData = (char *)malloc(total);
            td->TmpImage = XCreateImage(dpy, visual, depth, ZPixmap, 0, TmpData, td->fnt->twid, td->fnt->hgt, 8, 0);
        }
    }
#endif /* ! USE_XFT */
    return (0);
}

#endif /* USE_X11 */
