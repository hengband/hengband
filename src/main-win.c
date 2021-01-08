/*!
 * todo main関数を含むファイルの割に長過ぎる。main-win-utils.cなどといった形で分割したい
 * @file main-win.c
 * @brief Windows版固有実装(メインエントリポイント含む)
 * @date 2018/03/16
 * @author Hengband Team
 * @details
 *
 * <h3>概要</h3>
 * Windows98かその前後の頃を起点としたAPI実装。
 * 各種のゲームエンジンは無論、
 * DirectXといった昨今描画に標準的となったライブラリも用いていない。
 * タイルの描画処理などについては、現在動作の詳細を検証中。
 *
 * <h3>フォーク元の概要</h3>
 * <p>
 * Copyright (c) 1997 Ben Harrison, Skirmantas Kligys, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 * </p>
 * <p>
 * This file helps Angband work with Windows computers.
 *
 * To use this file, use an appropriate "Makefile" or "Project File",
 * make sure that "WINDOWS" and/or "WIN32" are defined somewhere, and
 * make sure to obtain various extra files as described below.
 *
 * The official compilation uses the CodeWarrior Pro compiler, which
 * includes a special project file and precompilable header file.
 * </p>
 *
 * <p>
 * The "lib/user/pref-win.prf" file contains keymaps, macro definitions,
 * and/or color redefinitions.
 * </p>
 *
 * <p>
 * The "lib/user/font-win.prf" contains attr/char mappings for use with the
 * normal "lib/xtra/font/*.fon" font files.
 * </p>
 *
 * <p>
 * The "lib/user/graf-win.prf" contains attr/char mappings for use with the
 * special "lib/xtra/graf/*.bmp" bitmap files, which are activated by a menu
 * item.
 * </p>
 *
 * <p>
 * Compiling this file, and using the resulting executable, requires
 * several extra files not distributed with the standard Angband code.
 * In any case, some "*.fon" files (including "8X13.FON" if nothing else)
 * must be placed into "lib/xtra/font/".  All of these extra files can be found
 * in the "ext-win" archive.
 * </p>
 *
 * <p>
 * The "term_xtra_win_clear()" function should probably do a low-level
 * clear of the current window, and redraw the borders and other things,
 * if only for efficiency.
 * </p>
 *
 * <p>
 * A simpler method is needed for selecting the "tile size" for windows.
 * </p>
 *
 * <p>
 * The various "warning" messages assume the existance of the "screen.w"
 * window, I think, and only a few calls actually check for its existance,
 * this may be okay since "NULL" means "on top of all windows". (?)  The
 * user must never be allowed to "hide" the main window, or the "menubar"
 * will disappear.
 * </p>
 *
 * <p>
 * Special "Windows Help Files" can be placed into "lib/xtra/help/" for
 * use with the "winhelp.exe" program.  These files *may* be available
 * at the ftp site somewhere, but I have not seen them.
 * </p>
 *
 * <p>
 * Initial framework (and most code) by Ben Harrison (benh@phial.com).
 *
 * Original code by Skirmantas Kligys (kligys@scf.usc.edu).
 *
 * Additional code by Ross E Becker (beckerr@cis.ohio-state.edu),
 * and Chris R. Martin (crm7479@tam2000.tamu.edu).
 * </p>
 */

#include "autopick/autopick-pref-processor.h"
#include "cmd-io/cmd-process-screen.h"
#include "cmd-io/cmd-save.h"
#include "core/game-play.h"
#include "core/player-processor.h"
#include "core/scores.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "dungeon/quest.h"
#include "floor/floor-base-definitions.h"
#include "floor/floor-events.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "io/record-play-movie.h"
#include "io/files-util.h"
#include "io/inet.h"
#include "io/input-key-acceptor.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "main/angband-initializer.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-lite.h"
#include "system/angband-version.h"
#include "system/angband.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "world/world.h"

#ifdef WINDOWS
#include "dungeon/dungeon.h"
#include "save/save.h"
#include <direct.h>
#include <locale.h>
#include <windows.h>

/*
 * Available graphic modes
 */
#define GRAPHICS_NONE 0
#define GRAPHICS_ORIGINAL 1
#define GRAPHICS_ADAM_BOLT 2
#define GRAPHICS_HENGBAND 3

/*
 * Menu constants -- see "ANGBAND.RC"
 */
#define IDM_FILE_NEW 100
#define IDM_FILE_OPEN 101
#define IDM_FILE_SAVE 110
#define IDM_FILE_SCORE 120
#define IDM_FILE_MOVIE 121
#define IDM_FILE_EXIT 130

#define IDM_WINDOW_VIS_0 200
#define IDM_WINDOW_VIS_1 201
#define IDM_WINDOW_VIS_2 202
#define IDM_WINDOW_VIS_3 203
#define IDM_WINDOW_VIS_4 204
#define IDM_WINDOW_VIS_5 205
#define IDM_WINDOW_VIS_6 206
#define IDM_WINDOW_VIS_7 207

#define IDM_WINDOW_FONT_0 210
#define IDM_WINDOW_FONT_1 211
#define IDM_WINDOW_FONT_2 212
#define IDM_WINDOW_FONT_3 213
#define IDM_WINDOW_FONT_4 214
#define IDM_WINDOW_FONT_5 215
#define IDM_WINDOW_FONT_6 216
#define IDM_WINDOW_FONT_7 217

#define IDM_WINDOW_POS_0 220
#define IDM_WINDOW_POS_1 221
#define IDM_WINDOW_POS_2 222
#define IDM_WINDOW_POS_3 223
#define IDM_WINDOW_POS_4 224
#define IDM_WINDOW_POS_5 225
#define IDM_WINDOW_POS_6 226
#define IDM_WINDOW_POS_7 227

#define IDM_WINDOW_BIZ_0 230
#define IDM_WINDOW_BIZ_1 231
#define IDM_WINDOW_BIZ_2 232
#define IDM_WINDOW_BIZ_3 233
#define IDM_WINDOW_BIZ_4 234
#define IDM_WINDOW_BIZ_5 235
#define IDM_WINDOW_BIZ_6 236
#define IDM_WINDOW_BIZ_7 237

#define IDM_WINDOW_I_WID_0 240
#define IDM_WINDOW_I_WID_1 241
#define IDM_WINDOW_I_WID_2 242
#define IDM_WINDOW_I_WID_3 243
#define IDM_WINDOW_I_WID_4 244
#define IDM_WINDOW_I_WID_5 245
#define IDM_WINDOW_I_WID_6 246
#define IDM_WINDOW_I_WID_7 247

#define IDM_WINDOW_D_WID_0 250
#define IDM_WINDOW_D_WID_1 251
#define IDM_WINDOW_D_WID_2 252
#define IDM_WINDOW_D_WID_3 253
#define IDM_WINDOW_D_WID_4 254
#define IDM_WINDOW_D_WID_5 255
#define IDM_WINDOW_D_WID_6 256
#define IDM_WINDOW_D_WID_7 257

#define IDM_WINDOW_I_HGT_0 260
#define IDM_WINDOW_I_HGT_1 261
#define IDM_WINDOW_I_HGT_2 262
#define IDM_WINDOW_I_HGT_3 263
#define IDM_WINDOW_I_HGT_4 264
#define IDM_WINDOW_I_HGT_5 265
#define IDM_WINDOW_I_HGT_6 266
#define IDM_WINDOW_I_HGT_7 267

#define IDM_WINDOW_D_HGT_0 270
#define IDM_WINDOW_D_HGT_1 271
#define IDM_WINDOW_D_HGT_2 272
#define IDM_WINDOW_D_HGT_3 273
#define IDM_WINDOW_D_HGT_4 274
#define IDM_WINDOW_D_HGT_5 275
#define IDM_WINDOW_D_HGT_6 276
#define IDM_WINDOW_D_HGT_7 277

#define IDM_OPTIONS_NO_GRAPHICS 400
#define IDM_OPTIONS_OLD_GRAPHICS 401
#define IDM_OPTIONS_NEW_GRAPHICS 402
#define IDM_OPTIONS_NEW2_GRAPHICS 403
#define IDM_OPTIONS_BIGTILE 409
#define IDM_OPTIONS_SOUND 410
#define IDM_OPTIONS_MUSIC 411
#define IDM_OPTIONS_SAVER 420
#define IDM_OPTIONS_MAP 430
#define IDM_OPTIONS_BG 440
#define IDM_OPTIONS_OPEN_BG 441

#define IDM_DUMP_SCREEN_HTML 450

#define IDM_HELP_CONTENTS 901

/*
 * Exclude parts of WINDOWS.H that are not needed (Win32)
 */
#define WIN32_LEAN_AND_MEAN
#define NONLS /* All NLS defines and routines */
#define NOSERVICE /* All Service Controller routines, SERVICE_ equates, etc. */
#define NOMCX /* Modem Configuration Extensions */

/*
 * Include the "windows" support file
 */
#include <windows.h>

/*
 * Exclude parts of MMSYSTEM.H that are not needed
 */
#define MMNODRV /* Installable driver support */
#define MMNOWAVE /* Waveform support */
#define MMNOMIDI /* MIDI support */
#define MMNOAUX /* Auxiliary audio support */
#define MMNOTIMER /* Timer support */
#define MMNOJOY /* Joystick support */
#define MMNOMCI /* MCI support */
#define MMNOMMIO /* Multimedia file I/O support */

#define INVALID_FILE_NAME (DWORD)0xFFFFFFFF
#define MOUSE_SENS 40

/*
 * Include some more files. Note: the Cygnus Cygwin compiler
 * doesn't use mmsystem.h instead it includes the winmm library
 * which performs a similar function.
 */
#include <commdlg.h>
#include <mmsystem.h>

/*
 * Include the support for loading bitmaps
 */
#include "term/readdib.h"

#define MoveTo(H, X, Y) MoveToEx(H, X, Y, NULL)

/*
 * Foreground color bits
 */
#define VID_BLACK 0x00
#define VID_BLUE 0x01
#define VID_GREEN 0x02
#define VID_CYAN 0x03
#define VID_RED 0x04
#define VID_MAGENTA 0x05
#define VID_YELLOW 0x06
#define VID_WHITE 0x07

/*
 * Bright text
 */
#define VID_BRIGHT 0x08

/*!
 * @struct term_data
 * @brief ターム情報構造体 / Extra "term" data
 * @details
 * <p>
 * pos_x / pos_y は各タームの左上点座標を指す。
 * </p>
 * <p>
 * tile_wid / tile_hgt は[ウィンドウ]メニューのタイルの幅/高さを～を
 * 1ドットずつ調整するステータスを指す。
 * また、フォントを変更すると都度自動調整される。
 * </p>
 * <p>
 * Note the use of "font_want" for the names of the font file requested by
 * the user, and the use of "font_file" for the currently active font file.
 *
 * The "font_file" is uppercased, and takes the form "8X13.FON", while
 * "font_want" can be in almost any form as long as it could be construed
 * as attempting to represent the name of a font.
 * </p>
 */
typedef struct {
    term_type t;
    concptr s;
    HWND w;
    DWORD dwStyle;
    DWORD dwExStyle;

    uint keys;
    TERM_LEN rows; /* int -> uint */
    TERM_LEN cols;

    uint pos_x; //!< タームの左上X座標
    uint pos_y; //!< タームの左上Y座標
    uint size_wid;
    uint size_hgt;
    uint size_ow1;
    uint size_oh1;
    uint size_ow2;
    uint size_oh2;

    bool size_hack;
    bool xtra_hack;
    bool visible;
    bool bizarre;
    concptr font_want;
    concptr font_file;
    HFONT font_id;
    int font_wid; //!< フォント横幅
    int font_hgt; //!< フォント縦幅
    int tile_wid; //!< タイル横幅
    int tile_hgt; //!< タイル縦幅

    uint map_tile_wid;
    uint map_tile_hgt;

    bool map_active;
    LOGFONT lf;

    bool posfix;
} term_data;

#define MAX_TERM_DATA 8 //!< Maximum number of windows

static term_data data[MAX_TERM_DATA]; //!< An array of term_data's
static term_data *my_td; //!< Hack -- global "window creation" pointer
POINT normsize; //!< Remember normal size of main window when maxmized

/*
 * was main window maximized on previous playing
 */
bool win_maximized = FALSE;

/*
 * game in progress
 */
bool game_in_progress = FALSE;

/*
 * note when "open"/"new" become valid
 */
bool initialized = FALSE;

/*
 * screen paletted, i.e. 256 colors
 */
bool paletted = FALSE;

/*
 * 16 colors screen, don't use RGB()
 */
bool colors16 = FALSE;

/*
 * Saved instance handle
 */
static HINSTANCE hInstance;

/*
 * Yellow brush for the cursor
 */
static HBRUSH hbrYellow;

/*
 * An icon
 */
static HICON hIcon;

/*
 * A palette
 */
static HPALETTE hPal;

/* bg */
static HBITMAP hBG = NULL;
static int use_bg = 0; //!< 背景使用フラグ、1なら私用。
static char bg_bitmap_file[1024] = "bg.bmp"; //!< 現在の背景ビットマップファイル名。

/*
 * The screen saver window
 */
static HWND hwndSaver;

/*!
 * 現在使用中のタイルID(0ならば未使用)
 * Flag set once "graphics" has been initialized
 */
static byte current_graphics_mode = 0;

/*
 * The global bitmap
 */
static DIBINIT infGraph;

/*
 * The global bitmap mask
 */
static DIBINIT infMask;

/*
 * Flag set once "sound" has been initialized
 */
static bool can_use_sound = FALSE;

#define SAMPLE_SOUND_MAX 16
/*
 * An array of sound file names
 */
static concptr sound_file[SOUND_MAX][SAMPLE_SOUND_MAX];

#define SAMPLE_MUSIC_MAX 16
static concptr music_file[MUSIC_BASIC_MAX][SAMPLE_MUSIC_MAX];
static concptr dungeon_music_file[1000][SAMPLE_MUSIC_MAX];
static concptr town_music_file[1000][SAMPLE_MUSIC_MAX];
static concptr quest_music_file[1000][SAMPLE_MUSIC_MAX];
static bool can_use_music = FALSE;

static MCI_OPEN_PARMS mop;
static char mci_device_type[256];

int current_music_type = 0;
int current_music_id = 0;

/*
 * Full path to ANGBAND.INI
 */
static concptr ini_file = NULL;

/*
 * Name of application
 */
static concptr AppName = "ANGBAND";

/*
 * Name of sub-window type
 */
static concptr AngList = "AngList";

/*
 * Directory names
 */
static concptr ANGBAND_DIR_XTRA_GRAF;
static concptr ANGBAND_DIR_XTRA_SOUND;
static concptr ANGBAND_DIR_XTRA_MUSIC;
static concptr ANGBAND_DIR_XTRA_HELP;
static concptr ANGBAND_DIR_XTRA_MUSIC;

/*
 * The "complex" color values
 */
static COLORREF win_clr[256];

/*
 * Flag for macro trigger with dump ASCII
 */
static bool term_no_press = FALSE;

/*
 * Copy and paste
 */
static bool mouse_down = FALSE;
static bool paint_rect = FALSE;
static TERM_LEN mousex = 0, mousey = 0;
static TERM_LEN oldx, oldy;

/*!
 * @brief The "simple" color values
 * @details
 * See "main-ibm.c" for original table information
 * The entries below are taken from the "color bits" defined above.
 * Note that many of the choices below suck, but so do crappy monitors.
 */
static BYTE win_pal[256] = {
    VID_BLACK, /* Dark */
    VID_WHITE, /* White */
    VID_CYAN, /* Slate XXX */
    VID_RED | VID_BRIGHT, /* Orange XXX */
    VID_RED, /* Red */
    VID_GREEN, /* Green */
    VID_BLUE, /* Blue */
    VID_YELLOW, /* Umber XXX */
    VID_BLACK | VID_BRIGHT, /* Light Dark */
    VID_CYAN | VID_BRIGHT, /* Light Slate XXX */
    VID_MAGENTA, /* Violet XXX */
    VID_YELLOW | VID_BRIGHT, /* Yellow */
    VID_MAGENTA | VID_BRIGHT, /* Light Red XXX */
    VID_GREEN | VID_BRIGHT, /* Light Green */
    VID_BLUE | VID_BRIGHT, /* Light Blue */
    VID_YELLOW /* Light Umber XXX */
};

/*
 * Hack -- define which keys are "special"
 */
static bool special_key[256];
static bool ignore_key[256];

/*
 * Hack -- initialization list for "special_key"
 */
static byte special_key_list[] = {
    VK_CLEAR, VK_PAUSE, VK_CAPITAL, VK_KANA, VK_JUNJA, VK_FINAL, VK_KANJI, VK_CONVERT, VK_NONCONVERT, VK_ACCEPT, VK_MODECHANGE, VK_PRIOR, VK_NEXT, VK_END,
    VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_SELECT, VK_PRINT, VK_EXECUTE, VK_SNAPSHOT, VK_INSERT, VK_DELETE, VK_HELP, VK_APPS, VK_NUMPAD0, VK_NUMPAD1,
    VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_MULTIPLY, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL,
    VK_DIVIDE, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18, VK_F19,
    VK_F20, VK_F21, VK_F22, VK_F23, VK_F24, VK_NUMLOCK, VK_SCROLL, VK_ATTN, VK_CRSEL, VK_EXSEL, VK_EREOF, VK_PLAY, VK_ZOOM, VK_NONAME, VK_PA1,
    0 /* End of List */
};

static byte ignore_key_list[] = {
    VK_ESCAPE, VK_TAB, VK_SPACE, 'F', 'W', 'O', /*'H',*/ /* these are menu characters.*/
    VK_SHIFT, VK_CONTROL, VK_MENU, VK_LWIN, VK_RWIN, VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU, 0 /* End of List */
};

/* Function prototype */

static bool is_already_running(void);

/* bg */
static void delete_bg(void)
{
    if (hBG != NULL) {
        DeleteObject(hBG);
        hBG = NULL;
    }
}

static int init_bg(void)
{
    char *bmfile = bg_bitmap_file;
    delete_bg();
    if (use_bg == 0)
        return 0;

    hBG = LoadImage(NULL, bmfile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hBG) {
        plog_fmt(_("壁紙用ビットマップ '%s' を読み込めません。", "Can't load the bitmap file '%s'."), bmfile);
        use_bg = 0;
        return 0;
    }

    use_bg = 1;
    return 1;
}

static void DrawBG(HDC hdc, RECT *r)
{
    if (!use_bg || !hBG)
        return;

    int x = r->left, y = r->top;
    int nx = x;
    int ny = y;
    BITMAP bm;
    GetObject(hBG, sizeof(bm), &bm);
    int swid = bm.bmWidth;
    int shgt = bm.bmHeight;

    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP hOld = SelectObject(hdcSrc, hBG);

    do {
        int sx = nx % swid;
        int cwid = MIN(swid - sx, r->right - nx);
        do {
            int sy = ny % shgt;
            int chgt = MIN(shgt - sy, r->bottom - ny);
            BitBlt(hdc, nx, ny, cwid, chgt, hdcSrc, sx, sy, SRCCOPY);
            ny += chgt;
        } while (ny < r->bottom);

        ny = y;
        nx += cwid;
    } while (nx < r->right);

    SelectObject(hdcSrc, hOld);
    DeleteDC(hdcSrc);
}

/*
 * Check for existance of a file
 */
static bool check_file(concptr s)
{
    char path[1024];
    strcpy(path, s);
    DWORD attrib = GetFileAttributes(path);
    if (attrib == INVALID_FILE_NAME)
        return FALSE;
    if (attrib & FILE_ATTRIBUTE_DIRECTORY)
        return FALSE;

    return TRUE;
}

/*
 * Check for existance of a directory
 */
static bool check_dir(concptr s)
{
    char path[1024];
    strcpy(path, s);
    int i = strlen(path);
    if (i && (path[i - 1] == '\\'))
        path[--i] = '\0';

    DWORD attrib = GetFileAttributes(path);
    if (attrib == INVALID_FILE_NAME)
        return FALSE;
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;

    return TRUE;
}

/*
 * Validate a file
 */
static void validate_file(concptr s)
{
    if (check_file(s))
        return;

    quit_fmt(_("必要なファイル[%s]が見あたりません。", "Cannot find required file:\n%s"), s);
}

/*
 * Validate a directory
 */
static void validate_dir(concptr s, bool vital)
{
    if (check_dir(s))
        return;

    if (vital) {
        quit_fmt(_("必要なディレクトリ[%s]が見あたりません。", "Cannot find required directory:\n%s"), s);
    } else if (mkdir(s)) {
        quit_fmt("Unable to create directory:\n%s", s);
    }
}

/*!
 * @brief (Windows版固有実装)Get the "size" for a window
 */
static void term_getsize(term_data *td)
{
    if (td->cols < 1)
        td->cols = 1;
    if (td->rows < 1)
        td->rows = 1;

    TERM_LEN wid = td->cols * td->tile_wid + td->size_ow1 + td->size_ow2;
    TERM_LEN hgt = td->rows * td->tile_hgt + td->size_oh1 + td->size_oh2;

    RECT rw, rc;
    if (td->w) {
        GetWindowRect(td->w, &rw);
        GetClientRect(td->w, &rc);

        td->size_wid = (rw.right - rw.left) - (rc.right - rc.left) + wid;
        td->size_hgt = (rw.bottom - rw.top) - (rc.bottom - rc.top) + hgt;

        td->pos_x = rw.left;
        td->pos_y = rw.top;
    } else {
        /* Tempolary calculation */
        rc.left = 0;
        rc.right = wid;
        rc.top = 0;
        rc.bottom = hgt;
        AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
        td->size_wid = rc.right - rc.left;
        td->size_hgt = rc.bottom - rc.top;
    }
}

/*
 * Write the "prefs" for a single term
 */
static void save_prefs_aux(int i)
{
    term_data *td = &data[i];
    GAME_TEXT sec_name[128];
    char buf[1024];

    if (!td->w)
        return;

    sprintf(sec_name, "Term-%d", i);

    if (i > 0) {
        strcpy(buf, td->visible ? "1" : "0");
        WritePrivateProfileString(sec_name, "Visible", buf, ini_file);
    }

#ifdef JP
    strcpy(buf, td->lf.lfFaceName[0] != '\0' ? td->lf.lfFaceName : "ＭＳ ゴシック");
#else
    strcpy(buf, td->lf.lfFaceName[0] != '\0' ? td->lf.lfFaceName : "Courier");
#endif

    WritePrivateProfileString(sec_name, "Font", buf, ini_file);

    wsprintf(buf, "%d", td->lf.lfWidth);
    WritePrivateProfileString(sec_name, "FontWid", buf, ini_file);
    wsprintf(buf, "%d", td->lf.lfHeight);
    WritePrivateProfileString(sec_name, "FontHgt", buf, ini_file);
    wsprintf(buf, "%d", td->lf.lfWeight);
    WritePrivateProfileString(sec_name, "FontWgt", buf, ini_file);

    strcpy(buf, td->bizarre ? "1" : "0");
    WritePrivateProfileString(sec_name, "Bizarre", buf, ini_file);

    wsprintf(buf, "%d", td->tile_wid);
    WritePrivateProfileString(sec_name, "TileWid", buf, ini_file);

    wsprintf(buf, "%d", td->tile_hgt);
    WritePrivateProfileString(sec_name, "TileHgt", buf, ini_file);

    WINDOWPLACEMENT lpwndpl;
    lpwndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(td->w, &lpwndpl);

    RECT rc = lpwndpl.rcNormalPosition;
    if (i == 0)
        wsprintf(buf, "%d", normsize.x);
    else
        wsprintf(buf, "%d", td->cols);

    WritePrivateProfileString(sec_name, "NumCols", buf, ini_file);

    if (i == 0)
        wsprintf(buf, "%d", normsize.y);
    else
        wsprintf(buf, "%d", td->rows);

    WritePrivateProfileString(sec_name, "NumRows", buf, ini_file);
    if (i == 0) {
        strcpy(buf, IsZoomed(td->w) ? "1" : "0");
        WritePrivateProfileString(sec_name, "Maximized", buf, ini_file);
    }

    GetWindowRect(td->w, &rc);
    wsprintf(buf, "%d", rc.left);
    WritePrivateProfileString(sec_name, "PositionX", buf, ini_file);

    wsprintf(buf, "%d", rc.top);
    WritePrivateProfileString(sec_name, "PositionY", buf, ini_file);
    if (i > 0) {
        strcpy(buf, td->posfix ? "1" : "0");
        WritePrivateProfileString(sec_name, "PositionFix", buf, ini_file);
    }
}

/*
 * Write the "prefs"
 * We assume that the windows have all been initialized
 */
static void save_prefs(void)
{
    char buf[128];
    sprintf(buf, "%d", arg_graphics);
    WritePrivateProfileString("Angband", "Graphics", buf, ini_file);

    strcpy(buf, arg_bigtile ? "1" : "0");
    WritePrivateProfileString("Angband", "Bigtile", buf, ini_file);

    strcpy(buf, arg_sound ? "1" : "0");
    WritePrivateProfileString("Angband", "Sound", buf, ini_file);

    strcpy(buf, arg_music ? "1" : "0");
    WritePrivateProfileString("Angband", "Music", buf, ini_file);

    strcpy(buf, use_bg ? "1" : "0");
    WritePrivateProfileString("Angband", "BackGround", buf, ini_file);
    WritePrivateProfileString("Angband", "BackGroundBitmap", bg_bitmap_file[0] != '\0' ? bg_bitmap_file : "bg.bmp", ini_file);

    for (int i = 0; i < MAX_TERM_DATA; ++i) {
        save_prefs_aux(i);
    }
}

/*
 * Load the "prefs" for a single term
 */
static void load_prefs_aux(int i)
{
    term_data *td = &data[i];
    GAME_TEXT sec_name[128];
    char tmp[1024];

    int dispx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int dispy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int posx = 0;
    int posy = 0;

    sprintf(sec_name, "Term-%d", i);
    sprintf(sec_name, "Term-%d", i);
    if (i > 0) {
        td->visible = (GetPrivateProfileInt(sec_name, "Visible", td->visible, ini_file) != 0);
    }

#ifdef JP
    GetPrivateProfileString(sec_name, "Font", "ＭＳ ゴシック", tmp, 127, ini_file);
#else
    GetPrivateProfileString(sec_name, "Font", "Courier", tmp, 127, ini_file);
#endif

    td->bizarre = (GetPrivateProfileInt(sec_name, "Bizarre", td->bizarre, ini_file) != 0);

    td->font_want = string_make(tmp);
    int hgt = 15;
    int wid = 0;
    td->lf.lfWidth = GetPrivateProfileInt(sec_name, "FontWid", wid, ini_file);
    td->lf.lfHeight = GetPrivateProfileInt(sec_name, "FontHgt", hgt, ini_file);
    td->lf.lfWeight = GetPrivateProfileInt(sec_name, "FontWgt", 0, ini_file);

    td->tile_wid = GetPrivateProfileInt(sec_name, "TileWid", td->lf.lfWidth, ini_file);
    td->tile_hgt = GetPrivateProfileInt(sec_name, "TileHgt", td->lf.lfHeight, ini_file);

    td->cols = GetPrivateProfileInt(sec_name, "NumCols", td->cols, ini_file);
    td->rows = GetPrivateProfileInt(sec_name, "NumRows", td->rows, ini_file);
    normsize.x = td->cols;
    normsize.y = td->rows;

    if (i == 0) {
        win_maximized = (GetPrivateProfileInt(sec_name, "Maximized", win_maximized, ini_file) != 0);
    }

    posx = GetPrivateProfileInt(sec_name, "PositionX", posx, ini_file);
    posy = GetPrivateProfileInt(sec_name, "PositionY", posy, ini_file);
    td->pos_x = MIN(MAX(0, posx), dispx - 128);
    td->pos_y = MIN(MAX(0, posy), dispy - 128);

    if (i > 0) {
        td->posfix = (GetPrivateProfileInt(sec_name, "PositionFix", td->posfix, ini_file) != 0);
    }
}

/*
 * Load the "prefs"
 */
static void load_prefs(void)
{
    arg_graphics = (byte)GetPrivateProfileInt("Angband", "Graphics", GRAPHICS_NONE, ini_file);
    arg_bigtile = (GetPrivateProfileInt("Angband", "Bigtile", FALSE, ini_file) != 0);
    use_bigtile = arg_bigtile;
    arg_sound = (GetPrivateProfileInt("Angband", "Sound", 0, ini_file) != 0);
    arg_music = (GetPrivateProfileInt("Angband", "Music", 0, ini_file) != 0);
    use_bg = GetPrivateProfileInt("Angband", "BackGround", 0, ini_file);
    GetPrivateProfileString("Angband", "BackGroundBitmap", "bg.bmp", bg_bitmap_file, 1023, ini_file);
    for (int i = 0; i < MAX_TERM_DATA; ++i) {
        load_prefs_aux(i);
    }
}

/*
 * - Taken from files.c.
 *
 * Extract "tokens" from a buffer
 *
 * This function uses "whitespace" as delimiters, and treats any amount of
 * whitespace as a single delimiter.  We will never return any empty tokens.
 * When given an empty buffer, or a buffer containing only "whitespace", we
 * will return no tokens.  We will never extract more than "num" tokens.
 *
 * By running a token through the "text_to_ascii()" function, you can allow
 * that token to include (encoded) whitespace, using "\s" to encode spaces.
 *
 * We save pointers to the tokens in "tokens", and return the number found.
 */
static s16b tokenize_whitespace(char *buf, s16b num, char **tokens)
{
    s16b k = 0;
    char *s = buf;

    while (k < num) {
        char *t;
        for (; *s && iswspace(*s); ++s) /* loop */
            ;

        if (!*s)
            break;

        for (t = s; *t && !iswspace(*t); ++t) /* loop */
            ;

        if (*t)
            *t++ = '\0';

        tokens[k++] = s;
        s = t;
    }

    return k;
}

static void load_sound_prefs(void)
{
    char tmp[1024];
    char ini_path[1024];
    char wav_path[1024];
    char *zz[SAMPLE_SOUND_MAX];

    path_build(ini_path, 1024, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
    for (int i = 0; i < SOUND_MAX; i++) {
        GetPrivateProfileString("Sound", angband_sound_name[i], "", tmp, 1024, ini_path);
        int num = tokenize_whitespace(tmp, SAMPLE_SOUND_MAX, zz);
        for (int j = 0; j < num; j++) {
            /* Access the sound */
            path_build(wav_path, 1024, ANGBAND_DIR_XTRA_SOUND, zz[j]);

            /* Save the sound filename, if it exists */
            if (check_file(wav_path))
                sound_file[i][j] = string_make(zz[j]);
        }
    }
}

static void load_music_prefs(void)
{
    char tmp[1024];
    char ini_path[1024];
    char wav_path[1024];
    char *zz[SAMPLE_MUSIC_MAX];
    char key[80];

    path_build(ini_path, 1024, ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
    GetPrivateProfileString("Device", "type", "", mci_device_type, 256, ini_path);
    for (int i = 0; i < MUSIC_BASIC_MAX; i++) {
        GetPrivateProfileString("Basic", angband_music_basic_name[i], "", tmp, 1024, ini_path);
        int num = tokenize_whitespace(tmp, SAMPLE_MUSIC_MAX, zz);
        for (int j = 0; j < num; j++) {
            path_build(wav_path, 1024, ANGBAND_DIR_XTRA_MUSIC, zz[j]);
            if (check_file(wav_path))
                music_file[i][j] = string_make(zz[j]);
        }
    }

    for (int i = 0; i < current_world_ptr->max_d_idx; i++) {
        sprintf(key, "dungeon%03d", i);
        GetPrivateProfileString("Dungeon", key, "", tmp, 1024, ini_path);
        int num = tokenize_whitespace(tmp, SAMPLE_MUSIC_MAX, zz);
        for (int j = 0; j < num; j++) {
            path_build(wav_path, 1024, ANGBAND_DIR_XTRA_MUSIC, zz[j]);
            if (check_file(wav_path))
                dungeon_music_file[i][j] = string_make(zz[j]);
        }
    }

    for (int i = 0; i < max_q_idx; i++) {
        sprintf(key, "quest%03d", i);
        GetPrivateProfileString("Quest", key, "", tmp, 1024, ini_path);
        int num = tokenize_whitespace(tmp, SAMPLE_MUSIC_MAX, zz);
        for (int j = 0; j < num; j++) {
            path_build(wav_path, 1024, ANGBAND_DIR_XTRA_MUSIC, zz[j]);
            if (check_file(wav_path))
                quest_music_file[i][j] = string_make(zz[j]);
        }
    }

    for (int i = 0; i < 1000; i++) /*!< @todo 町最大数指定 */
    {
        sprintf(key, "town%03d", i);
        GetPrivateProfileString("Town", key, "", tmp, 1024, ini_path);
        int num = tokenize_whitespace(tmp, SAMPLE_MUSIC_MAX, zz);
        for (int j = 0; j < num; j++) {
            path_build(wav_path, 1024, ANGBAND_DIR_XTRA_MUSIC, zz[j]);
            if (check_file(wav_path))
                town_music_file[i][j] = string_make(zz[j]);
        }
    }
}

/*
 * Create the new global palette based on the bitmap palette
 * (if any), and the standard 16 entry palette derived from
 * "win_clr[]" which is used for the basic 16 Angband colors.
 *
 * This function is never called before all windows are ready.
 *
 * This function returns FALSE if the new palette could not be
 * prepared, which should normally be a fatal error.  XXX XXX
 *
 * Note that only some machines actually use a "palette".
 */
static int new_palette(void)
{
    int i, nEntries;
    int pLogPalSize;
    int lppeSize;
    LPLOGPALETTE pLogPal;
    LPPALETTEENTRY lppe;
    term_data *td;
    if (!paletted)
        return TRUE;

    lppeSize = 0;
    lppe = NULL;
    nEntries = 0;

    HPALETTE hBmPal = infGraph.hPalette;
    if (hBmPal) {
        lppeSize = 256 * sizeof(PALETTEENTRY);
        lppe = (LPPALETTEENTRY)ralloc(lppeSize);
        nEntries = GetPaletteEntries(hBmPal, 0, 255, lppe);
        if ((nEntries == 0) || (nEntries > 220)) {
            plog(_("画面を16ビットか24ビットカラーモードにして下さい。", "Please switch to high- or true-color mode."));
            rnfree(lppe, lppeSize);
            return FALSE;
        }
    }

    pLogPalSize = sizeof(LOGPALETTE) + (nEntries + 16) * sizeof(PALETTEENTRY);
    pLogPal = (LPLOGPALETTE)ralloc(pLogPalSize);
    pLogPal->palVersion = 0x300;
    pLogPal->palNumEntries = nEntries + 16;
    for (i = 0; i < nEntries; i++) {
        pLogPal->palPalEntry[i] = lppe[i];
    }

    for (i = 0; i < 16; i++) {
        LPPALETTEENTRY p;
        p = &(pLogPal->palPalEntry[i + nEntries]);
        p->peRed = GetRValue(win_clr[i]);
        p->peGreen = GetGValue(win_clr[i]);
        p->peBlue = GetBValue(win_clr[i]);
        p->peFlags = PC_NOCOLLAPSE;
    }

    if (lppe)
        rnfree(lppe, lppeSize);

    HPALETTE hNewPal = CreatePalette(pLogPal);
    if (!hNewPal)
        quit(_("パレットを作成できません！", "Cannot create palette!"));

    rnfree(pLogPal, pLogPalSize);
    td = &data[0];
    HDC hdc = GetDC(td->w);
    SelectPalette(hdc, hNewPal, 0);
    i = RealizePalette(hdc);
    ReleaseDC(td->w, hdc);
    if (i == 0)
        quit(_("パレットをシステムエントリにマップできません！", "Cannot realize palette!"));

    for (i = 1; i < MAX_TERM_DATA; i++) {
        td = &data[i];
        hdc = GetDC(td->w);
        SelectPalette(hdc, hNewPal, 0);
        ReleaseDC(td->w, hdc);
    }

    if (hPal)
        DeleteObject(hPal);

    hPal = hNewPal;
    return TRUE;
}

/*!
 * @brief グラフィクスを初期化する / Initialize graphics
 * @details
 * <ul>
 * <li>メニュー[オプション]＞[グラフィクス]が「なし」以外の時に描画処理を初期化する。</li>
 * <li>呼び出されるタイミングはロード時、及び同メニューで「なし」以外に変更される毎になる。</li>
 * </ul>
 */
static bool init_graphics(void)
{
    char buf[1024];
    BYTE wid, hgt, twid, thgt, ox, oy;
    concptr name;

    if (arg_graphics == GRAPHICS_ADAM_BOLT) {
        wid = 16;
        hgt = 16;
        twid = 16;
        thgt = 16;
        ox = 0;
        oy = 0;
        name = "16X16.BMP";

        ANGBAND_GRAF = "new";
    } else if (arg_graphics == GRAPHICS_HENGBAND) {
        wid = 32;
        hgt = 32;
        twid = 32;
        thgt = 32;
        ox = 0;
        oy = 0;
        name = "32X32.BMP";

        ANGBAND_GRAF = "ne2";
    } else {
        wid = 8;
        hgt = 8;
        twid = 8;
        thgt = 8;
        ox = 0;
        oy = 0;
        name = "8X8.BMP";
        ANGBAND_GRAF = "old";
    }

    path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA_GRAF, name);
    if (!ReadDIB(data[0].w, buf, &infGraph)) {
        plog_fmt(_("ビットマップ '%s' を読み込めません。", "Cannot read bitmap file '%s'"), name);
        return FALSE;
    }

    infGraph.CellWidth = wid;
    infGraph.CellHeight = hgt;
    infGraph.TileWidth = twid;
    infGraph.TileHeight = thgt;
    infGraph.OffsetX = ox;
    infGraph.OffsetY = oy;

    if (arg_graphics == GRAPHICS_ADAM_BOLT) {
        path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA_GRAF, "mask.bmp");
        if (!ReadDIB(data[0].w, buf, &infMask)) {
            plog_fmt("Cannot read bitmap file '%s'", buf);
            return FALSE;
        }
    }

    if (arg_graphics == GRAPHICS_HENGBAND) {
        path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA_GRAF, "mask32.bmp");
        if (!ReadDIB(data[0].w, buf, &infMask)) {
            plog_fmt("Cannot read bitmap file '%s'", buf);
            return FALSE;
        }
    }

    if (!new_palette()) {
        plog(_("パレットを実現できません！", "Cannot activate palette!"));
        return FALSE;
    }

    current_graphics_mode = arg_graphics;
    return (current_graphics_mode);
}

/*
 * Initialize music
 */
static void init_music(void)
{
    if (!can_use_music) {
        load_music_prefs();
        can_use_music = TRUE;
    }
}

/*
 * Hack -- Stop a music
 */
static void stop_music(void)
{
    mciSendCommand(mop.wDeviceID, MCI_STOP, 0, 0);
    mciSendCommand(mop.wDeviceID, MCI_CLOSE, 0, 0);
}

/*
 * Initialize sound
 */
static void init_sound(void)
{
    if (!can_use_sound) {
        load_sound_prefs();
        can_use_sound = TRUE;
    }
}

/*
 * Resize a window
 */
static void term_window_resize(term_data *td)
{
    if (!td->w)
        return;

    SetWindowPos(td->w, 0, 0, 0, td->size_wid, td->size_hgt, SWP_NOMOVE | SWP_NOZORDER);
    InvalidateRect(td->w, NULL, TRUE);
}

/*
 * todo 引数のpathを消す
 * Force the use of a new "font file" for a term_data.
 * This function may be called before the "window" is ready.
 * This function returns zero only if everything succeeds.
 * Note that the "font name" must be capitalized!!!
 */
static errr term_force_font(term_data *td, concptr path)
{
    if (td->font_id)
        DeleteObject(td->font_id);

    (void)path;
    td->font_id = CreateFontIndirect(&(td->lf));
    int wid = td->lf.lfWidth;
    int hgt = td->lf.lfHeight;
    if (!td->font_id)
        return 1;

    if (!wid || !hgt) {
        HDC hdcDesktop;
        HFONT hfOld;
        TEXTMETRIC tm;

        hdcDesktop = GetDC(HWND_DESKTOP);
        hfOld = SelectObject(hdcDesktop, td->font_id);
        GetTextMetrics(hdcDesktop, &tm);
        SelectObject(hdcDesktop, hfOld);
        ReleaseDC(HWND_DESKTOP, hdcDesktop);

        wid = tm.tmAveCharWidth;
        hgt = tm.tmHeight;
    }

    td->font_wid = wid;
    td->font_hgt = hgt;

    return 0;
}

/*
 * Allow the user to change the font for this window.
 */
static void term_change_font(term_data *td)
{
    CHOOSEFONT cf;
    memset(&cf, 0, sizeof(cf));
    cf.lStructSize = sizeof(cf);
    cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
    cf.lpLogFont = &(td->lf);

    if (!ChooseFont(&cf))
        return;

    term_force_font(td, NULL);
    td->bizarre = TRUE;
    td->tile_wid = td->font_wid;
    td->tile_hgt = td->font_hgt;
    term_getsize(td);
    term_window_resize(td);
}

/*
 * Allow the user to lock this window.
 */
static void term_window_pos(term_data *td, HWND hWnd) { SetWindowPos(td->w, hWnd, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE); }

static void windows_map(player_type *player_ptr);

/*
 * Hack -- redraw a term_data
 */
static void term_data_redraw(player_type *player_ptr, term_data *td)
{
    if (td->map_active) {
        windows_map(player_ptr);
        return;
    }

    term_activate(&td->t);
    term_redraw();
    term_activate(term_screen);
}

void term_inversed_area(HWND hWnd, int x, int y, int w, int h)
{
    term_data *td = (term_data *)GetWindowLong(hWnd, 0);
    int tx = td->size_ow1 + x * td->tile_wid;
    int ty = td->size_oh1 + y * td->tile_hgt;
    int tw = w * td->tile_wid - 1;
    int th = h * td->tile_hgt - 1;

    HDC hdc = GetDC(hWnd);
    HBRUSH myBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = SelectObject(hdc, myBrush);
    HPEN oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));

    PatBlt(hdc, tx, ty, tw, th, PATINVERT);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
}

/*!
 * @brief //!< Windows版ユーザ設定項目実装部(実装必須) /Interact with the User
 */
static errr term_user_win(int n)
{
    (void)n;
    return 0;
}

/*
 * React to global changes
 */
static errr term_xtra_win_react(player_type *player_ptr)
{
    if (colors16) {
        for (int i = 0; i < 256; i++) {
            win_pal[i] = angband_color_table[i][0];
        }
    } else {
        COLORREF code;
        byte rv, gv, bv;
        bool change = FALSE;
        for (int i = 0; i < 256; i++) {
            rv = angband_color_table[i][1];
            gv = angband_color_table[i][2];
            bv = angband_color_table[i][3];
            code = PALETTERGB(rv, gv, bv);
            if (win_clr[i] != code) {
                change = TRUE;
                win_clr[i] = code;
            }
        }

        if (change)
            (void)new_palette();
    }

    if (use_sound != arg_sound) {
        init_sound();
        use_sound = arg_sound;
    }

    if (use_music != arg_music) {
        init_music();
        use_music = arg_music;
        if (!arg_music)
            stop_music();
        else
            select_floor_music(player_ptr);
    }

    if (use_graphics != arg_graphics) {
        if (arg_graphics && !init_graphics()) {
            plog(_("グラフィックスを初期化できません!", "Cannot initialize graphics!"));
            arg_graphics = GRAPHICS_NONE;
        }

        use_graphics = arg_graphics;
        reset_visuals(player_ptr, process_autopick_file_command);
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        term_type *old = Term;
        term_data *td = &data[i];
        if ((td->cols != td->t.wid) || (td->rows != td->t.hgt)) {
            term_activate(&td->t);
            term_resize(td->cols, td->rows);
            term_redraw();
            term_activate(old);
        }
    }

    return 0;
}

/*
 * Process at least one event
 */
static errr term_xtra_win_event(int v)
{
    MSG msg;
    if (v) {
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}

/*
 * Process all pending events
 */
static errr term_xtra_win_flush(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

/*
 * Hack -- clear the screen
 *
 * Make this more efficient
 */
static errr term_xtra_win_clear(void)
{
    term_data *td = (term_data *)(Term->data);

    RECT rc;
    rc.left = td->size_ow1;
    rc.right = rc.left + td->cols * td->tile_wid;
    rc.top = td->size_oh1;
    rc.bottom = rc.top + td->rows * td->tile_hgt;

    HDC hdc = GetDC(td->w);
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    if (use_bg) {
        rc.left = 0;
        rc.top = 0;
        DrawBG(hdc, &rc);
    }

    ReleaseDC(td->w, hdc);
    return 0;
}

/*
 * Hack -- make a noise
 */
static errr term_xtra_win_noise(void)
{
    MessageBeep(MB_ICONASTERISK);
    return 0;
}

/*
 * Hack -- make a sound
 */
static errr term_xtra_win_sound(int v)
{
    char buf[1024];
    if (!use_sound)
        return 1;
    if ((v < 0) || (v >= SOUND_MAX))
        return 1;

    int i;
    for (i = 0; i < SAMPLE_SOUND_MAX; i++) {
        if (!sound_file[v][i])
            break;
    }

    if (i == 0)
        return 1;

    path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, sound_file[v][Rand_external(i)]);
    return (PlaySound(buf, 0, SND_FILENAME | SND_ASYNC));
}

/*
 * Hack -- play a music
 */
static errr term_xtra_win_music(int n, int v)
{
    int i = 0;
    char buf[1024];
    if (n == TERM_XTRA_MUSIC_MUTE) {
        mciSendCommand(mop.wDeviceID, MCI_STOP, 0, 0);
        mciSendCommand(mop.wDeviceID, MCI_CLOSE, 0, 0);
    }

    if (!use_music)
        return 1;

    if (n == TERM_XTRA_MUSIC_BASIC && ((v < 0) || (v >= MUSIC_BASIC_MAX)))
        return 1;
    else if (v < 0 || v >= 1000)
        return (1); /*!< TODO */

    switch (n) {
    case TERM_XTRA_MUSIC_BASIC:
        for (i = 0; i < SAMPLE_MUSIC_MAX; i++)
            if (!music_file[v][i])
                break;
        break;
    case TERM_XTRA_MUSIC_DUNGEON:
        for (i = 0; i < SAMPLE_MUSIC_MAX; i++)
            if (!dungeon_music_file[v][i])
                break;
        break;
    case TERM_XTRA_MUSIC_QUEST:
        for (i = 0; i < SAMPLE_MUSIC_MAX; i++)
            if (!quest_music_file[v][i])
                break;
        break;
    case TERM_XTRA_MUSIC_TOWN:
        for (i = 0; i < SAMPLE_MUSIC_MAX; i++)
            if (!town_music_file[v][i])
                break;
        break;
    }

    if (i == 0) {
        return 1;
    }

    switch (n) {
    case TERM_XTRA_MUSIC_BASIC:
        path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, music_file[v][Rand_external(i)]);
        break;
    case TERM_XTRA_MUSIC_DUNGEON:
        path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, dungeon_music_file[v][Rand_external(i)]);
        break;
    case TERM_XTRA_MUSIC_QUEST:
        path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, quest_music_file[v][Rand_external(i)]);
        break;
    case TERM_XTRA_MUSIC_TOWN:
        path_build(buf, 1024, ANGBAND_DIR_XTRA_MUSIC, town_music_file[v][Rand_external(i)]);
        break;
    }

    if (current_music_type == n && current_music_id == v) {
        return 0;
    }
    current_music_type = n;
    current_music_id = v;

    mop.lpstrDeviceType = mci_device_type;
    mop.lpstrElementName = buf;
    mciSendCommand(mop.wDeviceID, MCI_STOP, 0, 0);
    mciSendCommand(mop.wDeviceID, MCI_CLOSE, 0, 0);
    mciSendCommand(mop.wDeviceID, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD)&mop);
    mciSendCommand(mop.wDeviceID, MCI_SEEK, MCI_SEEK_TO_START, 0);
    mciSendCommand(mop.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&mop);
    return 0;
}

/*
 * Delay for "x" milliseconds
 */
static int term_xtra_win_delay(int v)
{
    Sleep(v);
    return 0;
}

/*
 * todo z-termに影響があるのでplayer_typeの追加は保留
 * Do a "special thing"
 */
static errr term_xtra_win(int n, int v)
{
    switch (n) {
    case TERM_XTRA_NOISE: {
        return (term_xtra_win_noise());
    }
    case TERM_XTRA_MUSIC_BASIC:
    case TERM_XTRA_MUSIC_DUNGEON:
    case TERM_XTRA_MUSIC_QUEST:
    case TERM_XTRA_MUSIC_TOWN: {
        return (term_xtra_win_music(n, v));
    }
    case TERM_XTRA_SOUND: {
        return (term_xtra_win_sound(v));
    }
    case TERM_XTRA_BORED: {
        return (term_xtra_win_event(0));
    }
    case TERM_XTRA_EVENT: {
        return (term_xtra_win_event(v));
    }
    case TERM_XTRA_FLUSH: {
        return (term_xtra_win_flush());
    }
    case TERM_XTRA_CLEAR: {
        return (term_xtra_win_clear());
    }
    case TERM_XTRA_REACT: {
        return (term_xtra_win_react(p_ptr));
    }
    case TERM_XTRA_DELAY: {
        return (term_xtra_win_delay(v));
    }
    }

    return 1;
}

/*
 * Low level graphics (Assumes valid input).
 *
 * Draw a "cursor" at (x,y), using a "yellow box".
 */
static errr term_curs_win(int x, int y)
{
    term_data *td = (term_data *)(Term->data);
    int tile_wid, tile_hgt;
    if (td->map_active) {
        tile_wid = td->map_tile_wid;
        tile_hgt = td->map_tile_hgt;
    } else {
        tile_wid = td->tile_wid;
        tile_hgt = td->tile_hgt;
    }

    RECT rc;
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = GetDC(td->w);
    FrameRect(hdc, &rc, hbrYellow);
    ReleaseDC(td->w, hdc);
    return 0;
}

/*
 * Low level graphics (Assumes valid input).
 *
 * Draw a "big cursor" at (x,y), using a "yellow box".
 */
static errr term_bigcurs_win(int x, int y)
{
    term_data *td = (term_data *)(Term->data);
    int tile_wid, tile_hgt;
    if (td->map_active) {
        term_curs_win(x, y);
        return 0;
    } else {
        tile_wid = td->tile_wid;
        tile_hgt = td->tile_hgt;
    }

    RECT rc;
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + 2 * tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = GetDC(td->w);
    FrameRect(hdc, &rc, hbrYellow);
    ReleaseDC(td->w, hdc);
    return 0;
}

/*
 * Low level graphics (Assumes valid input).
 *
 * Erase a "block" of "n" characters starting at (x,y).
 */
static errr term_wipe_win(int x, int y, int n)
{
    term_data *td = (term_data *)(Term->data);
    RECT rc;
    rc.left = x * td->tile_wid + td->size_ow1;
    rc.right = rc.left + n * td->tile_wid;
    rc.top = y * td->tile_hgt + td->size_oh1;
    rc.bottom = rc.top + td->tile_hgt;

    HDC hdc = GetDC(td->w);
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    if (use_bg)
        DrawBG(hdc, &rc);
    else
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    ReleaseDC(td->w, hdc);
    return 0;
}

/*
 * Low level graphics.  Assumes valid input.
 *
 * Draw several ("n") chars, with an attr, at a given location.
 *
 * All "graphic" data is handled by "term_pict_win()", below.
 *
 * One would think there is a more efficient method for telling a window
 * what color it should be using to draw with, but perhaps simply changing
 * it every time is not too inefficient.
 */
static errr term_text_win(int x, int y, int n, TERM_COLOR a, concptr s)
{
    term_data *td = (term_data *)(Term->data);
    static HBITMAP WALL;
    static HBRUSH myBrush, oldBrush;
    static HPEN oldPen;
    static bool init_done = FALSE;

    if (!init_done) {
        WALL = LoadBitmap(hInstance, AppName);
        myBrush = CreatePatternBrush(WALL);
        init_done = TRUE;
    }

    RECT rc;
    rc.left = x * td->tile_wid + td->size_ow1;
    rc.right = rc.left + n * td->tile_wid;
    rc.top = y * td->tile_hgt + td->size_oh1;
    rc.bottom = rc.top + td->tile_hgt;

    HDC hdc = GetDC(td->w);
    SetBkColor(hdc, RGB(0, 0, 0));
    if (colors16) {
        SetTextColor(hdc, PALETTEINDEX(win_pal[a]));
    } else if (paletted) {
        SetTextColor(hdc, win_clr[a & 0x0F]);
    } else {
        SetTextColor(hdc, win_clr[a]);
    }

    SelectObject(hdc, td->font_id);
    if (use_bg)
        SetBkMode(hdc, TRANSPARENT);

    if (td->bizarre || (td->tile_hgt != td->font_hgt) || (td->tile_wid != td->font_wid)) {
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
        if (use_bg)
            DrawBG(hdc, &rc);

        rc.left += ((td->tile_wid - td->font_wid) / 2);
        rc.right = rc.left + td->font_wid;
        rc.top += ((td->tile_hgt - td->font_hgt) / 2);
        rc.bottom = rc.top + td->font_hgt;

        for (int i = 0; i < n; i++) {
#ifdef JP
            if (use_bigtile && *(s + i) == "■"[0] && *(s + i + 1) == "■"[1]) {
                rc.right += td->font_wid;
                oldBrush = SelectObject(hdc, myBrush);
                oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
                Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                rc.right -= td->font_wid;
                i++;
                rc.left += 2 * td->tile_wid;
                rc.right += 2 * td->tile_wid;
            } else if (iskanji(*(s + i))) /* 2バイト文字 */
            {
                rc.right += td->font_wid;
                ExtTextOut(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 2, NULL);
                rc.right -= td->font_wid;
                i++;
                rc.left += 2 * td->tile_wid;
                rc.right += 2 * td->tile_wid;
            } else if (*(s + i) == 127) {
                oldBrush = SelectObject(hdc, myBrush);
                oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
                Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                rc.left += td->tile_wid;
                rc.right += td->tile_wid;
            } else {
                ExtTextOut(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 1, NULL);
                rc.left += td->tile_wid;
                rc.right += td->tile_wid;
            }
#else
            if (*(s + i) == 127) {
                oldBrush = SelectObject(hdc, myBrush);
                oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
                Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                rc.left += td->tile_wid;
                rc.right += td->tile_wid;
            } else {
                ExtTextOut(hdc, rc.left, rc.top, ETO_CLIPPED, &rc, s + i, 1, NULL);
                rc.left += td->tile_wid;
                rc.right += td->tile_wid;
            }
#endif
        }
    } else {
        ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED, &rc, s, n, NULL);
    }

    ReleaseDC(td->w, hdc);
    return 0;
}

/*
 * Low level graphics.  Assumes valid input.
 *
 * Draw an array of "special" attr/char pairs at the given location.
 *
 * We use the "term_pict_win()" function for "graphic" data, which are
 * encoded by setting the "high-bits" of both the "attr" and the "char"
 * data.  We use the "attr" to represent the "row" of the main bitmap,
 * and the "char" to represent the "col" of the main bitmap.  The use
 * of this function is induced by the "higher_pict" flag.
 *
 * If "graphics" is not available, we simply "wipe" the given grids.
 */
static errr term_pict_win(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, concptr cp, const TERM_COLOR *tap, concptr tcp)
{
    term_data *td = (term_data *)(Term->data);
    int i;
    HDC hdcMask = NULL;
    if (!use_graphics) {
        return (term_wipe_win(x, y, n));
    }

    TERM_LEN w1 = infGraph.CellWidth;
    TERM_LEN h1 = infGraph.CellHeight;
    TERM_LEN tw1 = infGraph.TileWidth;
    TERM_LEN th1 = infGraph.TileHeight;
    TERM_LEN w2, h2, tw2 = 0;
    if (td->map_active) {
        w2 = td->map_tile_wid;
        h2 = td->map_tile_hgt;
    } else {
        w2 = td->tile_wid;
        h2 = td->tile_hgt;
        tw2 = w2;
        if (use_bigtile)
            tw2 *= 2;
    }

    TERM_LEN x2 = x * w2 + td->size_ow1 + infGraph.OffsetX;
    TERM_LEN y2 = y * h2 + td->size_oh1 + infGraph.OffsetY;
    HDC hdc = GetDC(td->w);
    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP hbmSrcOld = SelectObject(hdcSrc, infGraph.hBitmap);

    if (arg_graphics == GRAPHICS_ADAM_BOLT || arg_graphics == GRAPHICS_HENGBAND) {
        hdcMask = CreateCompatibleDC(hdc);
        SelectObject(hdcMask, infMask.hBitmap);
    }

    for (i = 0; i < n; i++, x2 += w2) {
        TERM_COLOR a = ap[i];
        char c = cp[i];
        int row = (a & 0x7F);
        int col = (c & 0x7F);
        TERM_LEN x1 = col * w1;
        TERM_LEN y1 = row * h1;

        if (arg_graphics == GRAPHICS_ADAM_BOLT || arg_graphics == GRAPHICS_HENGBAND) {
            TERM_LEN x3 = (tcp[i] & 0x7F) * w1;
            TERM_LEN y3 = (tap[i] & 0x7F) * h1;
            tw2 = tw2 * w1 / tw1;
            h2 = h2 * h1 / th1;
            if ((tw1 == tw2) && (th1 == h2)) {
                BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x3, y3, SRCCOPY);
                BitBlt(hdc, x2, y2, tw2, h2, hdcMask, x1, y1, SRCAND);
                BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, SRCPAINT);
                continue;
            }

            SetStretchBltMode(hdc, COLORONCOLOR);
            StretchBlt(hdc, x2, y2, tw2, h2, hdcMask, x3, y3, w1, h1, SRCAND);
            StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x3, y3, w1, h1, SRCPAINT);
            if ((x1 != x3) || (y1 != y3)) {
                StretchBlt(hdc, x2, y2, tw2, h2, hdcMask, x1, y1, w1, h1, SRCAND);
                StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, w1, h1, SRCPAINT);
            }

            continue;
        }

        if ((w1 == tw2) && (h1 == h2)) {
            BitBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, SRCCOPY);
            continue;
        }

        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchBlt(hdc, x2, y2, tw2, h2, hdcSrc, x1, y1, w1, h1, SRCCOPY);
    }

    SelectObject(hdcSrc, hbmSrcOld);
    DeleteDC(hdcSrc);
    if (arg_graphics == GRAPHICS_ADAM_BOLT || arg_graphics == GRAPHICS_HENGBAND) {
        SelectObject(hdcMask, hbmSrcOld);
        DeleteDC(hdcMask);
    }

    ReleaseDC(td->w, hdc);
    return 0;
}

static void windows_map(player_type *player_ptr)
{
    term_data *td = &data[0];
    TERM_COLOR ta;
    if (!use_graphics)
        return;

    term_xtra_win_clear();
    td->map_tile_wid = (td->tile_wid * td->cols) / MAX_WID;
    td->map_tile_hgt = (td->tile_hgt * td->rows) / MAX_HGT;
    td->map_active = TRUE;

    TERM_LEN min_x = 0;
    TERM_LEN min_y = 0;
    TERM_LEN max_x = player_ptr->current_floor_ptr->width;
    TERM_LEN max_y = player_ptr->current_floor_ptr->height;

    char c;
    for (TERM_LEN x = min_x; x < max_x; x++) {
        for (TERM_LEN y = min_y; y < max_y; y++) {
            TERM_COLOR a;
            char tc;
            map_info(player_ptr, y, x, &a, (char *)&c, &ta, (char *)&tc);
            if ((a & 0x80) && (c & 0x80)) {
                term_pict_win(x - min_x, y - min_y, 1, &a, &c, &ta, &tc);
            }
        }
    }

    term_curs_win(player_ptr->x - min_x, player_ptr->y - min_y);
    term_inkey(&c, TRUE, TRUE);
    term_flush();
    td->map_active = FALSE;
    term_xtra_win_clear();
    term_redraw();
}

/*
 * Create and initialize a "term_data" given a title
 */
static void term_data_link(term_data *td)
{
    term_type *t = &td->t;
    term_init(t, td->cols, td->rows, td->keys);
    t->soft_cursor = TRUE;
    t->higher_pict = TRUE;
    t->attr_blank = TERM_WHITE;
    t->char_blank = ' ';
    t->user_hook = term_user_win;
    t->xtra_hook = term_xtra_win;
    t->curs_hook = term_curs_win;
    t->bigcurs_hook = term_bigcurs_win;
    t->wipe_hook = term_wipe_win;
    t->text_hook = term_text_win;
    t->pict_hook = term_pict_win;
    t->data = (vptr)(td);
}

/*
 * Create the windows
 *
 * First, instantiate the "default" values, then read the "ini_file"
 * to over-ride selected values, then create the windows, and fonts.
 *
 * Must use SW_SHOW not SW_SHOWNA, since on 256 color display
 * must make active to realize the palette.
 */
static void init_windows(void)
{
    term_data *td;
    td = &data[0];
    WIPE(td, term_data);
#ifdef JP
    td->s = "変愚蛮怒";
#else
    td->s = angband_term_name[0];
#endif

    td->keys = 1024;
    td->rows = 24;
    td->cols = 80;
    td->visible = TRUE;
    td->size_ow1 = 2;
    td->size_ow2 = 2;
    td->size_oh1 = 2;
    td->size_oh2 = 2;
    td->pos_x = 7 * 30;
    td->pos_y = 7 * 20;
    td->posfix = FALSE;
    td->bizarre = TRUE;

    for (int i = 1; i < MAX_TERM_DATA; i++) {
        td = &data[i];
        WIPE(td, term_data);
        td->s = angband_term_name[i];
        td->keys = 16;
        td->rows = 24;
        td->cols = 80;
        td->visible = FALSE;
        td->size_ow1 = 1;
        td->size_ow2 = 1;
        td->size_oh1 = 1;
        td->size_oh2 = 1;
        td->pos_x = (7 - i) * 30;
        td->pos_y = (7 - i) * 20;
        td->posfix = FALSE;
        td->bizarre = TRUE;
    }

    load_prefs();

    /* Atrributes of main window */
    td = &data[0];
    td->dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_VISIBLE);
    td->dwExStyle = 0;
    td->visible = TRUE;

    /* Attributes of sub windows */
    for (int i = 1; i < MAX_TERM_DATA; i++) {
        td = &data[i];
        td->dwStyle = (WS_OVERLAPPED | WS_THICKFRAME | WS_SYSMENU);
        td->dwExStyle = (WS_EX_TOOLWINDOW);
    }

    /* Font of each window */
    for (int i = 0; i < MAX_TERM_DATA; i++) {
        td = &data[i];
        strncpy(td->lf.lfFaceName, td->font_want, LF_FACESIZE);
        td->lf.lfCharSet = DEFAULT_CHARSET;
        td->lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
        term_force_font(td, NULL);
        if (!td->tile_wid)
            td->tile_wid = td->font_wid;
        if (!td->tile_hgt)
            td->tile_hgt = td->font_hgt;
        term_getsize(td);
        term_window_resize(td);
    }

    /* Create sub windows */
    for (int i = MAX_TERM_DATA - 1; i >= 1; --i) {
        td = &data[i];

        my_td = td;
        td->w = CreateWindowEx(
            td->dwExStyle, AngList, td->s, td->dwStyle, td->pos_x, td->pos_y, td->size_wid, td->size_hgt, HWND_DESKTOP, NULL, hInstance, NULL);
        my_td = NULL;

        if (!td->w)
            quit(_("サブウィンドウに作成に失敗しました", "Failed to create sub-window"));

        td->size_hack = TRUE;
        term_getsize(td);
        term_window_resize(td);

        if (td->visible) {
            ShowWindow(td->w, SW_SHOW);
        }
        td->size_hack = FALSE;

        term_data_link(td);
        angband_term[i] = &td->t;

        if (td->visible) {
            /* Activate the window */
            SetActiveWindow(td->w);
        }

        if (td->posfix) {
            term_window_pos(td, HWND_TOPMOST);
        } else {
            term_window_pos(td, td->w);
        }
    }

    /* Create main window */
    td = &data[0];
    my_td = td;
    td->w = CreateWindowEx(td->dwExStyle, AppName, td->s, td->dwStyle, td->pos_x, td->pos_y, td->size_wid, td->size_hgt, HWND_DESKTOP, NULL, hInstance, NULL);
    my_td = NULL;

    if (!td->w)
        quit(_("メインウィンドウの作成に失敗しました", "Failed to create Angband window"));

    /* Resize */
    td->size_hack = TRUE;
    term_getsize(td);
    term_window_resize(td);
    td->size_hack = FALSE;

    term_data_link(td);
    angband_term[0] = &td->t;
    normsize.x = td->cols;
    normsize.y = td->rows;

    if (win_maximized)
        ShowWindow(td->w, SW_SHOWMAXIMIZED);
    else
        ShowWindow(td->w, SW_SHOW);

    SetWindowPos(td->w, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    (void)new_palette();
    hbrYellow = CreateSolidBrush(win_clr[TERM_YELLOW]);
    (void)term_xtra_win_flush();
}

/*
 * Prepare the menus
 */
static void setup_menus(void)
{
    HMENU hm = GetMenu(data[0].w);
    EnableMenuItem(hm, IDM_FILE_NEW, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_FILE_OPEN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_FILE_EXIT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_FILE_SCORE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

    if (!current_world_ptr->character_generated) {
        EnableMenuItem(hm, IDM_FILE_NEW, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hm, IDM_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
    }

    if (current_world_ptr->character_generated) {
        EnableMenuItem(hm, IDM_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);
    }

    EnableMenuItem(hm, IDM_FILE_EXIT, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hm, IDM_FILE_SCORE, MF_BYCOMMAND | MF_ENABLED);

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_VIS_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        CheckMenuItem(hm, IDM_WINDOW_VIS_0 + i, (data[i].visible ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hm, IDM_WINDOW_VIS_0 + i, MF_BYCOMMAND | MF_ENABLED);
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_FONT_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_FONT_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_POS_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        CheckMenuItem(hm, IDM_WINDOW_POS_0 + i, (data[i].posfix ? MF_CHECKED : MF_UNCHECKED));
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_POS_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_BIZ_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        CheckMenuItem(hm, IDM_WINDOW_BIZ_0 + i, (data[i].bizarre ? MF_CHECKED : MF_UNCHECKED));
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_BIZ_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_I_WID_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_I_WID_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_D_WID_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_D_WID_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_I_HGT_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_I_HGT_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        EnableMenuItem(hm, IDM_WINDOW_D_HGT_0 + i, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        if (data[i].visible) {
            EnableMenuItem(hm, IDM_WINDOW_D_HGT_0 + i, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    EnableMenuItem(hm, IDM_OPTIONS_NO_GRAPHICS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_OPTIONS_OLD_GRAPHICS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_OPTIONS_NEW_GRAPHICS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_OPTIONS_BIGTILE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hm, IDM_OPTIONS_SOUND, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
#ifdef JP
#else
    EnableMenuItem(hm, IDM_OPTIONS_SAVER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
#endif

    if (use_graphics != GRAPHICS_NONE)
        EnableMenuItem(GetMenu(data[0].w), IDM_OPTIONS_MAP, MF_BYCOMMAND | MF_ENABLED);
    else
        EnableMenuItem(GetMenu(data[0].w), IDM_OPTIONS_MAP, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

    CheckMenuItem(hm, IDM_OPTIONS_NO_GRAPHICS, (arg_graphics == GRAPHICS_NONE ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_OLD_GRAPHICS, (arg_graphics == GRAPHICS_ORIGINAL ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_NEW_GRAPHICS, (arg_graphics == GRAPHICS_ADAM_BOLT ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_NEW2_GRAPHICS, (arg_graphics == GRAPHICS_HENGBAND ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_BIGTILE, (arg_bigtile ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_MUSIC, (arg_music ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_SOUND, (arg_sound ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_BG, (use_bg ? MF_CHECKED : MF_UNCHECKED));
#ifdef JP
#else
    CheckMenuItem(hm, IDM_OPTIONS_SAVER, (hwndSaver ? MF_CHECKED : MF_UNCHECKED));
#endif
    EnableMenuItem(hm, IDM_OPTIONS_NO_GRAPHICS, MF_ENABLED);
    EnableMenuItem(hm, IDM_OPTIONS_OLD_GRAPHICS, MF_ENABLED);
    EnableMenuItem(hm, IDM_OPTIONS_NEW_GRAPHICS, MF_ENABLED);
    EnableMenuItem(hm, IDM_OPTIONS_BIGTILE, MF_ENABLED);
    EnableMenuItem(hm, IDM_OPTIONS_SOUND, MF_ENABLED);
    EnableMenuItem(hm, IDM_OPTIONS_SAVER, MF_BYCOMMAND | MF_ENABLED);
}

/*
 * Check for double clicked (or dragged) savefile
 *
 * Apparently, Windows copies the entire filename into the first
 * piece of the "command line string".  Perhaps we should extract
 * the "basename" of that filename and append it to the "save" dir.
 */
static void check_for_save_file(player_type *player_ptr, LPSTR cmd_line)
{
    char *s;
    s = cmd_line;
    if (!*s)
        return;

    strcat(savefile, s);
    validate_file(savefile);
    game_in_progress = TRUE;
    play_game(player_ptr, FALSE, FALSE);
}

/*
 * Process a menu command
 */
static void process_menus(player_type *player_ptr, WORD wCmd)
{
    term_data *td;
    OPENFILENAME ofn;
    switch (wCmd) {
    case IDM_FILE_NEW: {
        if (!initialized) {
            plog(_("まだ初期化中です...", "You cannot do that yet..."));
        } else if (game_in_progress) {
            plog(_("プレイ中は新しいゲームを始めることができません！", "You can't start a new game while you're still playing!"));
        } else {
            game_in_progress = TRUE;
            term_flush();
            play_game(player_ptr, TRUE, FALSE);
            quit(NULL);
        }

        break;
    }
    case IDM_FILE_OPEN: {
        if (!initialized) {
            plog(_("まだ初期化中です...", "You cannot do that yet..."));
        } else if (game_in_progress) {
            plog(_("プレイ中はゲームをロードすることができません！", "You can't open a new game while you're still playing!"));
        } else {
            memset(&ofn, 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = data[0].w;
            ofn.lpstrFilter = "Save Files (*.)\0*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFile = savefile;
            ofn.nMaxFile = 1024;
            ofn.lpstrInitialDir = ANGBAND_DIR_SAVE;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

            if (GetOpenFileName(&ofn)) {
                validate_file(savefile);
                game_in_progress = TRUE;
                term_flush();
                play_game(player_ptr, FALSE, FALSE);
                quit(NULL);
            }
        }

        break;
    }
    case IDM_FILE_SAVE: {
        if (game_in_progress && current_world_ptr->character_generated) {
            if (!can_save) {
                plog(_("今はセーブすることは出来ません。", "You may not do that right now."));
                break;
            }

            msg_flag = FALSE;
            do_cmd_save_game(player_ptr, FALSE);
        } else {
            plog(_("今、セーブすることは出来ません。", "You may not do that right now."));
        }

        break;
    }
    case IDM_FILE_EXIT: {
        if (game_in_progress && current_world_ptr->character_generated) {
            if (!can_save) {
                plog(_("今は終了できません。", "You may not do that right now."));
                break;
            }

            msg_flag = FALSE;
            forget_lite(player_ptr->current_floor_ptr);
            forget_view(player_ptr->current_floor_ptr);
            clear_mon_lite(player_ptr->current_floor_ptr);

            term_key_push(SPECIAL_KEY_QUIT);
            break;
        }

        quit(NULL);
        break;
    }
    case IDM_FILE_SCORE: {
        char buf[1024];
        path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
        highscore_fd = fd_open(buf, O_RDONLY);
        if (highscore_fd < 0) {
            msg_print("Score file unavailable.");
        } else {
            screen_save();
            term_clear();
            display_scores_aux(0, MAX_HISCORES, -1, NULL);
            (void)fd_close(highscore_fd);
            highscore_fd = -1;
            screen_load();
            term_fresh();
        }

        break;
    }
    case IDM_FILE_MOVIE: {
        if (!initialized) {
            plog(_("まだ初期化中です...", "You cannot do that yet..."));
        } else if (game_in_progress) {
            plog(_("プレイ中はムービーをロードすることができません！", "You can't open a movie while you're playing!"));
        } else {
            memset(&ofn, 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = data[0].w;
            ofn.lpstrFilter = "Angband Movie Files (*.amv)\0*.amv\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFile = savefile;
            ofn.nMaxFile = 1024;
            ofn.lpstrInitialDir = ANGBAND_DIR_USER;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

            if (GetOpenFileName(&ofn)) {
                prepare_browse_movie_without_path_build(savefile);
                play_game(player_ptr, FALSE, TRUE);
                quit(NULL);
                return;
            }
        }

        break;
    }
    case IDM_WINDOW_VIS_0: {
        plog(_("メインウィンドウは非表示にできません！", "You are not allowed to do that!"));
        break;
    }
    case IDM_WINDOW_VIS_1:
    case IDM_WINDOW_VIS_2:
    case IDM_WINDOW_VIS_3:
    case IDM_WINDOW_VIS_4:
    case IDM_WINDOW_VIS_5:
    case IDM_WINDOW_VIS_6:
    case IDM_WINDOW_VIS_7: {
        int i = wCmd - IDM_WINDOW_VIS_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        if (!td->visible) {
            td->visible = TRUE;
            ShowWindow(td->w, SW_SHOW);
            term_data_redraw(player_ptr, td);
        } else {
            td->visible = FALSE;
            td->posfix = FALSE;
            ShowWindow(td->w, SW_HIDE);
        }

        break;
    }
    case IDM_WINDOW_FONT_0:
    case IDM_WINDOW_FONT_1:
    case IDM_WINDOW_FONT_2:
    case IDM_WINDOW_FONT_3:
    case IDM_WINDOW_FONT_4:
    case IDM_WINDOW_FONT_5:
    case IDM_WINDOW_FONT_6:
    case IDM_WINDOW_FONT_7: {
        int i = wCmd - IDM_WINDOW_FONT_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        term_change_font(td);
        break;
    }
    case IDM_WINDOW_POS_1:
    case IDM_WINDOW_POS_2:
    case IDM_WINDOW_POS_3:
    case IDM_WINDOW_POS_4:
    case IDM_WINDOW_POS_5:
    case IDM_WINDOW_POS_6:
    case IDM_WINDOW_POS_7: {
        int i = wCmd - IDM_WINDOW_POS_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        if (!td->posfix && td->visible) {
            td->posfix = TRUE;
            term_window_pos(td, HWND_TOPMOST);
        } else {
            td->posfix = FALSE;
            term_window_pos(td, data[0].w);
        }

        break;
    }
    case IDM_WINDOW_BIZ_0:
    case IDM_WINDOW_BIZ_1:
    case IDM_WINDOW_BIZ_2:
    case IDM_WINDOW_BIZ_3:
    case IDM_WINDOW_BIZ_4:
    case IDM_WINDOW_BIZ_5:
    case IDM_WINDOW_BIZ_6:
    case IDM_WINDOW_BIZ_7: {
        int i = wCmd - IDM_WINDOW_BIZ_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        td->bizarre = !td->bizarre;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_I_WID_0:
    case IDM_WINDOW_I_WID_1:
    case IDM_WINDOW_I_WID_2:
    case IDM_WINDOW_I_WID_3:
    case IDM_WINDOW_I_WID_4:
    case IDM_WINDOW_I_WID_5:
    case IDM_WINDOW_I_WID_6:
    case IDM_WINDOW_I_WID_7: {
        int i = wCmd - IDM_WINDOW_I_WID_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        td->tile_wid += 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_D_WID_0:
    case IDM_WINDOW_D_WID_1:
    case IDM_WINDOW_D_WID_2:
    case IDM_WINDOW_D_WID_3:
    case IDM_WINDOW_D_WID_4:
    case IDM_WINDOW_D_WID_5:
    case IDM_WINDOW_D_WID_6:
    case IDM_WINDOW_D_WID_7: {
        int i = wCmd - IDM_WINDOW_D_WID_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        td->tile_wid -= 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_I_HGT_0:
    case IDM_WINDOW_I_HGT_1:
    case IDM_WINDOW_I_HGT_2:
    case IDM_WINDOW_I_HGT_3:
    case IDM_WINDOW_I_HGT_4:
    case IDM_WINDOW_I_HGT_5:
    case IDM_WINDOW_I_HGT_6:
    case IDM_WINDOW_I_HGT_7: {
        int i = wCmd - IDM_WINDOW_I_HGT_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        td->tile_hgt += 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_WINDOW_D_HGT_0:
    case IDM_WINDOW_D_HGT_1:
    case IDM_WINDOW_D_HGT_2:
    case IDM_WINDOW_D_HGT_3:
    case IDM_WINDOW_D_HGT_4:
    case IDM_WINDOW_D_HGT_5:
    case IDM_WINDOW_D_HGT_6:
    case IDM_WINDOW_D_HGT_7: {
        int i = wCmd - IDM_WINDOW_D_HGT_0;
        if ((i < 0) || (i >= MAX_TERM_DATA))
            break;

        td = &data[i];
        td->tile_hgt -= 1;
        term_getsize(td);
        term_window_resize(td);
        break;
    }
    case IDM_OPTIONS_NO_GRAPHICS: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        if (arg_graphics != GRAPHICS_NONE) {
            arg_graphics = GRAPHICS_NONE;
            term_xtra_win_react(player_ptr);
            term_key_push(KTRL('R'));
        }

        break;
    }
    case IDM_OPTIONS_OLD_GRAPHICS: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        if (arg_graphics != GRAPHICS_ORIGINAL) {
            arg_graphics = GRAPHICS_ORIGINAL;
            term_xtra_win_react(player_ptr);
            term_key_push(KTRL('R'));
        }

        break;
    }
    case IDM_OPTIONS_NEW_GRAPHICS: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        if (arg_graphics != GRAPHICS_ADAM_BOLT) {
            arg_graphics = GRAPHICS_ADAM_BOLT;
            term_xtra_win_react(player_ptr);
            term_key_push(KTRL('R'));
        }

        break;
    }
    case IDM_OPTIONS_NEW2_GRAPHICS: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        if (arg_graphics != GRAPHICS_HENGBAND) {
            arg_graphics = GRAPHICS_HENGBAND;
            term_xtra_win_react(player_ptr);
            term_key_push(KTRL('R'));
        }

        break;
    }
    case IDM_OPTIONS_BIGTILE: {
        td = &data[0];
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        arg_bigtile = !arg_bigtile;
        term_activate(&td->t);
        term_resize(td->cols, td->rows);
        InvalidateRect(td->w, NULL, TRUE);
        break;
    }
    case IDM_OPTIONS_MUSIC: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        arg_music = !arg_music;
        term_xtra_win_react(player_ptr);
        term_key_push(KTRL('R'));
        break;
    }
    case IDM_OPTIONS_SOUND: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        arg_sound = !arg_sound;
        term_xtra_win_react(player_ptr);
        term_key_push(KTRL('R'));
        break;
    }
    case IDM_OPTIONS_BG: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        use_bg = !use_bg;
        init_bg();
        term_xtra_win_react(player_ptr);
        term_key_push(KTRL('R'));
        break;
    }
    case IDM_OPTIONS_OPEN_BG: {
        if (!inkey_flag) {
            plog("You may not do that right now.");
            break;
        }

        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = data[0].w;
        ofn.lpstrFilter = "Bitmap Files (*.bmp)\0*.bmp\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = bg_bitmap_file;
        ofn.nMaxFile = 1023;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = _("壁紙を選んでね。", "Choose wall paper.");
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

        if (GetOpenFileName(&ofn)) {
            use_bg = 1;
            init_bg();
        }

        term_xtra_win_react(player_ptr);
        term_key_push(KTRL('R'));
        break;
    }
    case IDM_DUMP_SCREEN_HTML: {
        static char buf[1024] = "";
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = data[0].w;
        ofn.lpstrFilter = "HTML Files (*.html)\0*.html\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = buf;
        ofn.nMaxFile = 1023;
        ofn.lpstrDefExt = "html";
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = _("HTMLでスクリーンダンプを保存", "Save screen dump as HTML.");
        ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn)) {
            do_cmd_save_screen_html_aux(buf, 0);
        }

        break;
    }
    case IDM_OPTIONS_SAVER: {
        if (hwndSaver) {
            DestroyWindow(hwndSaver);
            hwndSaver = NULL;
            break;
        }

        hwndSaver = CreateWindowEx(WS_EX_TOPMOST, "WindowsScreenSaverClass", "Angband Screensaver", WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0, 0,
            GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);

        if (hwndSaver) {
            SetWindowPos(hwndSaver, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        } else {
            plog(_("ウィンドウを作成出来ません", "Failed to create saver window"));
        }

        break;
    }
    case IDM_OPTIONS_MAP: {
        windows_map(player_ptr);
        break;
    }

    case IDM_HELP_CONTENTS: {
        char buf[1024];
        char tmp[1024];
        path_build(tmp, sizeof(tmp), ANGBAND_DIR_XTRA_HELP, "zangband.hlp");
        if (check_file(tmp)) {
            sprintf(buf, "winhelp.exe %s", tmp);
            WinExec(buf, SW_NORMAL);
            break;
        }

        plog_fmt(_("ヘルプファイル[%s]が見付かりません。", "Cannot find help file: %s"), tmp);
        plog(_("代わりにオンラインヘルプを使用してください。", "Use the online help files instead."));
        break;
    }
    }
}

/*
 * Add a keypress to the "queue"
 */
static errr term_keypress(int k)
{
    /* Refuse to enqueue non-keys */
    if (!k)
        return -1;

    /* Store the char, advance the queue */
    Term->key_queue[Term->key_head++] = (char)k;

    /* Circular queue, handle wrap */
    if (Term->key_head == Term->key_size)
        Term->key_head = 0;

    if (Term->key_head != Term->key_tail)
        return 0;

    return 1;
}

static bool process_keydown(WPARAM wParam, LPARAM lParam)
{
    bool mc = FALSE;
    bool ms = FALSE;
    bool ma = FALSE;

    if (GetKeyState(VK_CONTROL) & 0x8000)
        mc = TRUE;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        ms = TRUE;
    if (GetKeyState(VK_MENU) & 0x8000)
        ma = TRUE;

    term_no_press = (ma) ? TRUE : FALSE;
    if (special_key[(byte)(wParam)] || (ma && !ignore_key[(byte)(wParam)])) {
        bool ext_key = (lParam & 0x1000000L) ? TRUE : FALSE;
        bool numpad = FALSE;

        term_keypress(31);
        if (mc)
            term_keypress('C');
        if (ms)
            term_keypress('S');
        if (ma)
            term_keypress('A');

        int i = LOBYTE(HIWORD(lParam));
        term_keypress('x');
        switch (wParam) {
        case VK_DIVIDE:
            term_no_press = TRUE;
        case VK_RETURN:
            numpad = ext_key;
            break;
        case VK_NUMPAD0:
        case VK_NUMPAD1:
        case VK_NUMPAD2:
        case VK_NUMPAD3:
        case VK_NUMPAD4:
        case VK_NUMPAD5:
        case VK_NUMPAD6:
        case VK_NUMPAD7:
        case VK_NUMPAD8:
        case VK_NUMPAD9:
        case VK_ADD:
        case VK_MULTIPLY:
        case VK_SUBTRACT:
        case VK_SEPARATOR:
        case VK_DECIMAL:
            term_no_press = TRUE;
        case VK_CLEAR:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_INSERT:
        case VK_DELETE:
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
            numpad = !ext_key;
        }

        if (numpad)
            term_keypress('K');

        term_keypress(hexsym[i / 16]);
        term_keypress(hexsym[i % 16]);
        term_keypress(13);

        return 1;
    }

    return 0;
}

/*!
 * todo WNDCLASSに影響があるのでplayer_type*の追加は保留
 */
LRESULT PASCAL AngbandWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    term_data *td;
    td = (term_data *)GetWindowLong(hWnd, 0);

    switch (uMsg) {
    case WM_NCCREATE: {
        SetWindowLong(hWnd, 0, (LONG)(my_td));
        break;
    }
    case WM_CREATE: {
        mop.dwCallback = (DWORD)hWnd;
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO *lpmmi;
        RECT rc;

        lpmmi = (MINMAXINFO *)lParam;
        if (!td)
            return 1;

        rc.left = rc.top = 0;
        rc.right = rc.left + 80 * td->tile_wid + td->size_ow1 + td->size_ow2;
        rc.bottom = rc.top + 24 * td->tile_hgt + td->size_oh1 + td->size_oh2 + 1;

        AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);

        lpmmi->ptMinTrackSize.x = rc.right - rc.left;
        lpmmi->ptMinTrackSize.y = rc.bottom - rc.top;

        return 0;
    }
    case WM_PAINT: {
        BeginPaint(hWnd, &ps);
        if (td)
            term_data_redraw(p_ptr, td);
        EndPaint(hWnd, &ps);
        ValidateRect(hWnd, NULL);
        return 0;
    }
    case MM_MCINOTIFY: {
        if (wParam == MCI_NOTIFY_SUCCESSFUL) {
            mciSendCommand(mop.wDeviceID, MCI_SEEK, MCI_SEEK_TO_START, 0);
            mciSendCommand(mop.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&mop);
        }

        return 0;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        if (process_keydown(wParam, lParam))
            return 0;
        break;
    }
    case WM_CHAR: {
        if (term_no_press)
            term_no_press = FALSE;
        else
            term_keypress(wParam);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        mousex = MIN(LOWORD(lParam) / td->tile_wid, td->cols - 1);
        mousey = MIN(HIWORD(lParam) / td->tile_hgt, td->rows - 1);
        mouse_down = TRUE;
        oldx = mousex;
        oldy = mousey;
        return 0;
    }
    case WM_LBUTTONUP: {
        HGLOBAL hGlobal;
        LPSTR lpStr;
        TERM_LEN dx = abs(oldx - mousex) + 1;
        TERM_LEN dy = abs(oldy - mousey) + 1;
        TERM_LEN ox = (oldx > mousex) ? mousex : oldx;
        TERM_LEN oy = (oldy > mousey) ? mousey : oldy;

        mouse_down = FALSE;
        paint_rect = FALSE;

#ifdef JP
        int sz = (dx + 3) * dy;
#else
        int sz = (dx + 2) * dy;
#endif
        hGlobal = GlobalAlloc(GHND, sz + 1);
        if (hGlobal == NULL)
            return 0;
        lpStr = (LPSTR)GlobalLock(hGlobal);

        for (int i = 0; i < dy; i++) {
#ifdef JP
            char *s;
            char **scr = data[0].t.scr->c;

            C_MAKE(s, (dx + 1), char);
            strncpy(s, &scr[oy + i][ox], dx);

            if (ox > 0) {
                if (iskanji(scr[oy + i][ox - 1]))
                    s[0] = ' ';
            }

            if (ox + dx < data[0].cols) {
                if (iskanji(scr[oy + i][ox + dx - 1]))
                    s[dx - 1] = ' ';
            }

            for (int j = 0; j < dx; j++) {
                if (s[j] == 127)
                    s[j] = '#';
                *lpStr++ = s[j];
            }
#else
            for (int j = 0; j < dx; j++) {
                *lpStr++ = data[0].t.scr->c[oy + i][ox + j];
            }
#endif
            if (dy > 1) {
                *lpStr++ = '\r';
                *lpStr++ = '\n';
            }
        }

        GlobalUnlock(hGlobal);
        if (OpenClipboard(hWnd) == 0) {
            GlobalFree(hGlobal);
            return 0;
        }

        EmptyClipboard();
        SetClipboardData(CF_TEXT, hGlobal);
        CloseClipboard();
        term_redraw();
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (!mouse_down)
            return 0;

        int dx, dy;
        int cx = MIN(LOWORD(lParam) / td->tile_wid, td->cols - 1);
        int cy = MIN(HIWORD(lParam) / td->tile_hgt, td->rows - 1);
        int ox, oy;

        if (paint_rect) {
            dx = abs(oldx - mousex) + 1;
            dy = abs(oldy - mousey) + 1;
            ox = (oldx > mousex) ? mousex : oldx;
            oy = (oldy > mousey) ? mousey : oldy;
            term_inversed_area(hWnd, ox, oy, dx, dy);
        } else {
            paint_rect = TRUE;
        }

        dx = abs(cx - mousex) + 1;
        dy = abs(cy - mousey) + 1;
        ox = (cx > mousex) ? mousex : cx;
        oy = (cy > mousey) ? mousey : cy;
        term_inversed_area(hWnd, ox, oy, dx, dy);

        oldx = cx;
        oldy = cy;
        return 0;
    }
    case WM_INITMENU: {
        setup_menus();
        return 0;
    }
    case WM_CLOSE: {
        if (!game_in_progress || !current_world_ptr->character_generated) {
            quit(NULL);
            return 0;
        }

        if (!can_save) {
            plog(_("今は終了できません。", "You may not do that right now."));
            return 0;
        }

        msg_flag = FALSE;
        forget_lite(p_ptr->current_floor_ptr);
        forget_view(p_ptr->current_floor_ptr);
        clear_mon_lite(p_ptr->current_floor_ptr);
        term_key_push(SPECIAL_KEY_QUIT);
        return 0;
    }
    case WM_QUERYENDSESSION: {
        if (!game_in_progress || !current_world_ptr->character_generated) {
            quit(NULL);
            return 0;
        }

        msg_flag = FALSE;
        if (p_ptr->chp < 0)
            p_ptr->is_dead = FALSE;
        exe_write_diary(p_ptr, DIARY_GAMESTART, 0, _("----ゲーム中断----", "---- Save and Exit Game ----"));

        p_ptr->panic_save = 1;
        signals_ignore_tstp();
        (void)strcpy(p_ptr->died_from, _("(緊急セーブ)", "(panic save)"));
        (void)save_player(p_ptr);
        quit(NULL);
        return 0;
    }
    case WM_QUIT: {
        quit(NULL);
        return 0;
    }
    case WM_COMMAND: {
        process_menus(p_ptr, LOWORD(wParam));
        return 0;
    }
    case WM_SIZE: {
        if (!td)
            return 1;
        if (!td->w)
            return 1;
        if (td->size_hack)
            return 1;

        // todo 二重のswitch文。後で分割する.
        switch (wParam) {
        case SIZE_MINIMIZED: {
            for (int i = 1; i < MAX_TERM_DATA; i++) {
                if (data[i].visible)
                    ShowWindow(data[i].w, SW_HIDE);
            }

            return 0;
        }
        case SIZE_MAXIMIZED:
        case SIZE_RESTORED: {
            TERM_LEN cols = (LOWORD(lParam) - td->size_ow1) / td->tile_wid;
            TERM_LEN rows = (HIWORD(lParam) - td->size_oh1) / td->tile_hgt;
            if ((td->cols != cols) || (td->rows != rows)) {
                td->cols = cols;
                td->rows = rows;
                if (!IsZoomed(td->w) && !IsIconic(td->w)) {
                    normsize.x = td->cols;
                    normsize.y = td->rows;
                }

                term_activate(&td->t);
                term_resize(td->cols, td->rows);
                InvalidateRect(td->w, NULL, TRUE);
            }

            td->size_hack = TRUE;
            for (int i = 1; i < MAX_TERM_DATA; i++) {
                if (data[i].visible)
                    ShowWindow(data[i].w, SW_SHOW);
            }

            td->size_hack = FALSE;

            return 0;
        }
        }

        break;
    }
    case WM_PALETTECHANGED: {
        if ((HWND)wParam == hWnd)
            return 0;
    }
    case WM_QUERYNEWPALETTE: {
        if (!paletted)
            return 0;

        HDC hdc = GetDC(hWnd);
        SelectPalette(hdc, hPal, FALSE);
        int i = RealizePalette(hdc);
        if (i)
            InvalidateRect(hWnd, NULL, TRUE);

        ReleaseDC(hWnd, hdc);
        return 0;
    }
    case WM_ACTIVATE: {
        if (!wParam || HIWORD(lParam))
            break;

        for (int i = 1; i < MAX_TERM_DATA; i++) {
            if (!data[i].posfix)
                term_window_pos(&data[i], hWnd);
        }

        SetFocus(hWnd);
        return 0;
    }
    case WM_ACTIVATEAPP: {
        if (IsIconic(td->w))
            break;

        for (int i = 1; i < MAX_TERM_DATA; i++) {
            if (data[i].visible) {
                if (wParam == TRUE) {
                    ShowWindow(data[i].w, SW_SHOW);
                } else {
                    ShowWindow(data[i].w, SW_HIDE);
                }
            }
        }
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*!
 * todo WNDCLASSに影響があるのでplayer_type*の追加は保留
 */
LRESULT PASCAL AngbandListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    term_data *td;
    PAINTSTRUCT ps;
    td = (term_data *)GetWindowLong(hWnd, 0);

    switch (uMsg) {
    case WM_NCCREATE: {
        SetWindowLong(hWnd, 0, (LONG)(my_td));
        break;
    }
    case WM_CREATE: {
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO *lpmmi;
        RECT rc;

        lpmmi = (MINMAXINFO *)lParam;
        if (!td)
            return 1;

        rc.left = rc.top = 0;
        rc.right = rc.left + 20 * td->tile_wid + td->size_ow1 + td->size_ow2;
        rc.bottom = rc.top + 3 * td->tile_hgt + td->size_oh1 + td->size_oh2 + 1;

        AdjustWindowRectEx(&rc, td->dwStyle, TRUE, td->dwExStyle);
        lpmmi->ptMinTrackSize.x = rc.right - rc.left;
        lpmmi->ptMinTrackSize.y = rc.bottom - rc.top;
        return 0;
    }
    case WM_SIZE: {
        if (!td)
            return 1;
        if (!td->w)
            return 1;
        if (td->size_hack)
            return 1;

        td->size_hack = TRUE;

        TERM_LEN cols = (LOWORD(lParam) - td->size_ow1) / td->tile_wid;
        TERM_LEN rows = (HIWORD(lParam) - td->size_oh1) / td->tile_hgt;
        if ((td->cols != cols) || (td->rows != rows)) {
            term_type *old_term = Term;
            td->cols = cols;
            td->rows = rows;
            term_activate(&td->t);
            term_resize(td->cols, td->rows);
            term_activate(old_term);
            InvalidateRect(td->w, NULL, TRUE);
            p_ptr->window = 0xFFFFFFFF;
            handle_stuff(p_ptr);
        }

        td->size_hack = FALSE;
        return 0;
    }
    case WM_PAINT: {
        BeginPaint(hWnd, &ps);
        if (td)
            term_data_redraw(p_ptr, td);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        if (process_keydown(wParam, lParam))
            return 0;

        break;
    }
    case WM_CHAR: {
        if (term_no_press)
            term_no_press = FALSE;
        else
            term_keypress(wParam);
        return 0;
    }
    case WM_PALETTECHANGED: {
        if ((HWND)wParam == hWnd)
            return FALSE;
    }
    case WM_QUERYNEWPALETTE: {
        if (!paletted)
            return 0;

        HDC hdc = GetDC(hWnd);
        SelectPalette(hdc, hPal, FALSE);
        int i = RealizePalette(hdc);
        if (i)
            InvalidateRect(hWnd, NULL, TRUE);

        ReleaseDC(hWnd, hdc);
        return 0;
    }
    case WM_NCLBUTTONDOWN: {
        if (wParam == HTCLOSE)
            wParam = HTSYSMENU;

        if (wParam == HTSYSMENU) {
            if (td->visible) {
                td->visible = FALSE;
                ShowWindow(td->w, SW_HIDE);
            }

            return 0;
        }

        break;
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT PASCAL AngbandSaverProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int iMouse = 0;
    static WORD xMouse = 0;
    static WORD yMouse = 0;

    switch (uMsg) {
    case WM_NCCREATE: {
        break;
    }

    case WM_SETCURSOR: {
        SetCursor(NULL);
        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_KEYDOWN: {
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (iMouse) {
            int dx = LOWORD(lParam) - xMouse;
            int dy = HIWORD(lParam) - yMouse;

            if (dx < 0)
                dx = -dx;
            if (dy < 0)
                dy = -dy;

            if ((dx > MOUSE_SENS) || (dy > MOUSE_SENS)) {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
            }
        }

        iMouse = 1;
        xMouse = LOWORD(lParam);
        yMouse = HIWORD(lParam);

        return 0;
    }
    case WM_CLOSE: {
        DestroyWindow(hwndSaver);
        hwndSaver = NULL;
        return 0;
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 * Display warning message (see "z-util.c")
 */
static void hack_plog(concptr str)
{
    if (str) {
#ifdef JP
        MessageBox(NULL, str, "警告！", MB_ICONEXCLAMATION | MB_OK);
#else
        MessageBox(NULL, str, "Warning", MB_ICONEXCLAMATION | MB_OK);
#endif
    }
}

/*
 * Display error message and quit (see "z-util.c")
 */
static void hack_quit(concptr str)
{
    if (str) {
#ifdef JP
        MessageBox(NULL, str, "エラー！", MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
#else
        MessageBox(NULL, str, "Error", MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
#endif
    }

    UnregisterClass(AppName, hInstance);
    if (hIcon)
        DestroyIcon(hIcon);

    exit(0);
}

/*
 * Display warning message (see "z-util.c")
 */
static void hook_plog(concptr str)
{
    if (str) {
#ifdef JP
        MessageBox(data[0].w, str, "警告！", MB_ICONEXCLAMATION | MB_OK);
#else
        MessageBox(data[0].w, str, "Warning", MB_ICONEXCLAMATION | MB_OK);
#endif
    }
}

/*
 * Display error message and quit (see "z-util.c")
 */
static void hook_quit(concptr str)
{
    if (str) {
#ifdef JP
        MessageBox(data[0].w, str, "エラー！", MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
#else
        MessageBox(data[0].w, str, "Error", MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
#endif
    }

    save_prefs();
    for (int i = MAX_TERM_DATA - 1; i >= 0; --i) {
        term_force_font(&data[i], NULL);
        if (data[i].font_want)
            string_free(data[i].font_want);
        if (data[i].w)
            DestroyWindow(data[i].w);
        data[i].w = 0;
    }

    if (infGraph.hPalette)
        DeleteObject(infGraph.hPalette);
    if (infGraph.hBitmap)
        DeleteObject(infGraph.hBitmap);
    if (infMask.hPalette)
        DeleteObject(infMask.hPalette);
    if (infMask.hBitmap)
        DeleteObject(infMask.hBitmap);

    DeleteObject(hbrYellow);
    delete_bg();

    if (hPal)
        DeleteObject(hPal);

    UnregisterClass(AppName, hInstance);
    if (hIcon)
        DestroyIcon(hIcon);

    exit(0);
}

/*
 * Init some stuff
 */
static void init_stuff(void)
{
    char path[1024];
    GetModuleFileName(hInstance, path, 512);
    argv0 = path;
    strcpy(path + strlen(path) - 4, ".INI");
    ini_file = string_make(path);
    int i = strlen(path);

    for (; i > 0; i--) {
        if (path[i] == '\\') {
            break;
        }
    }

    strcpy(path + i + 1, "lib\\");
    validate_dir(path, TRUE);
    init_file_paths(path, path);
    validate_dir(ANGBAND_DIR_APEX, FALSE);
    validate_dir(ANGBAND_DIR_BONE, FALSE);
    if (!check_dir(ANGBAND_DIR_EDIT)) {
        validate_dir(ANGBAND_DIR_DATA, TRUE);
    } else {
        validate_dir(ANGBAND_DIR_DATA, FALSE);
    }

    validate_dir(ANGBAND_DIR_FILE, TRUE);
    validate_dir(ANGBAND_DIR_HELP, FALSE);
    validate_dir(ANGBAND_DIR_INFO, FALSE);
    validate_dir(ANGBAND_DIR_PREF, TRUE);
    validate_dir(ANGBAND_DIR_SAVE, FALSE);
    validate_dir(ANGBAND_DIR_USER, TRUE);
    validate_dir(ANGBAND_DIR_XTRA, TRUE);
    path_build(path, sizeof(path), ANGBAND_DIR_FILE, _("news_j.txt", "news.txt"));

    validate_file(path);
    path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "graf");
    ANGBAND_DIR_XTRA_GRAF = string_make(path);
    validate_dir(ANGBAND_DIR_XTRA_GRAF, TRUE);

    path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "sound");
    ANGBAND_DIR_XTRA_SOUND = string_make(path);
    validate_dir(ANGBAND_DIR_XTRA_SOUND, FALSE);

    path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "music");
    ANGBAND_DIR_XTRA_MUSIC = string_make(path);
    validate_dir(ANGBAND_DIR_XTRA_MUSIC, FALSE);

    path_build(path, sizeof(path), ANGBAND_DIR_XTRA, "help");
    ANGBAND_DIR_XTRA_HELP = string_make(path);
}

/*!
 * todo よく見るとhMutexはちゃんと使われていない……？
 * @brief (Windows固有)変愚蛮怒が起動済かどうかのチェック
 */
static bool is_already_running(void)
{
    HANDLE hMutex;
    hMutex = CreateMutex(NULL, TRUE, VERSION_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief (Windows固有)Windowsアプリケーションとしてのエントリポイント
 */
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc;
    HDC hdc;
    MSG msg;

    setlocale(LC_ALL, "ja_JP");
    (void)nCmdShow;
    hInstance = hInst;
    if (is_already_running()) {
        MessageBox(
            NULL, _("変愚蛮怒はすでに起動しています。", "Hengband is already running."), _("エラー！", "Error"), MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    if (hPrevInst == NULL) {
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = AngbandWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 4;
        wc.hInstance = hInst;
        wc.hIcon = hIcon = LoadIcon(hInst, AppName);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName = AppName;
        wc.lpszClassName = AppName;

        if (!RegisterClass(&wc))
            exit(1);

        wc.lpfnWndProc = AngbandListProc;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = AngList;

        if (!RegisterClass(&wc))
            exit(2);

        wc.style = CS_VREDRAW | CS_HREDRAW | CS_SAVEBITS | CS_DBLCLKS;
        wc.lpfnWndProc = AngbandSaverProc;
        wc.hCursor = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "WindowsScreenSaverClass";

        if (!RegisterClass(&wc))
            exit(3);
    }

    plog_aux = hack_plog;
    quit_aux = hack_quit;
    core_aux = hack_quit;

    init_stuff();
    for (int i = 0; special_key_list[i]; ++i) {
        special_key[special_key_list[i]] = TRUE;
    }

    for (int i = 0; ignore_key_list[i]; ++i) {
        ignore_key[ignore_key_list[i]] = TRUE;
    }

    hdc = GetDC(NULL);
    colors16 = (GetDeviceCaps(hdc, BITSPIXEL) == 4);
    paletted = ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) ? TRUE : FALSE);
    ReleaseDC(NULL, hdc);

    for (int i = 0; i < 256; i++) {
        byte rv = angband_color_table[i][1];
        byte gv = angband_color_table[i][2];
        byte bv = angband_color_table[i][3];
        win_clr[i] = PALETTERGB(rv, gv, bv);
        angband_color_table[i][0] = win_pal[i];
    }

    init_windows();
    init_bg();

    plog_aux = hook_plog;
    quit_aux = hook_quit;
    core_aux = hook_quit;

    ANGBAND_SYS = "win";
    if (7 != GetKeyboardType(0))
        ANGBAND_KEYBOARD = "0";
    else {
        switch (GetKeyboardType(1)) {
        case 0x0D01:
        case 0x0D02:
        case 0x0D03:
        case 0x0D04:
        case 0x0D05:
        case 0x0D06:
            /* NEC PC-98x1 */
            ANGBAND_KEYBOARD = "NEC98";
            break;
        default:
            /* PC/AT */
            ANGBAND_KEYBOARD = "JAPAN";
        }
    }

    signals_init();
    term_activate(term_screen);
    init_angband(p_ptr, process_autopick_file_command);
    initialized = TRUE;
    check_for_save_file(p_ptr, lpCmdLine);
    prt(_("[ファイル] メニューの [新規] または [開く] を選択してください。", "[Choose 'New' or 'Open' from the 'File' menu]"), 23, _(8, 17));
    term_fresh();
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    quit(NULL);
    return 0;
}
#endif /* WINDOWS */
