﻿/*!
 * @file main-win.cpp
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
 * The "lib/user/font-win.prf" contains attr/char mappings for wall.bmp.
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
 * All of these extra files can be found in the "ext-win" archive.
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
 * Initial framework (and most code) by Ben Harrison (benh@phial.com).
 *
 * Original code by Skirmantas Kligys (kligys@scf.usc.edu).
 *
 * Additional code by Ross E Becker (beckerr@cis.ohio-state.edu),
 * and Chris R. Martin (crm7479@tam2000.tamu.edu).
 * </p>
 */

#ifdef WINDOWS

#include "cmd-io/cmd-save.h"
#include "cmd-visual/cmd-draw.h"
#include "core/game-play.h"
#include "core/player-processor.h"
#include "core/scores.h"
#include "core/special-internal-keys.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "core/window-redrawer.h"
#include "floor/floor-events.h"
#include "game-option/runtime-arguments.h"
#include "game-option/special-options.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/record-play-movie.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "main-win/graphics-win.h"
#include "main-win/main-win-bg.h"
#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-mci.h"
#include "main-win/main-win-menuitem.h"
#include "main-win/main-win-music.h"
#include "main-win/main-win-sound.h"
#include "main-win/main-win-utils.h"
#include "main/angband-initializer.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-lite.h"
#include "save/save.h"
#include "system/angband.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "wizard/spoiler-util.h"
#include "wizard/wizard-spoiler.h"
#include "world/world.h"

#include <cstdlib>
#include <locale>
#include <vector>

#include <commdlg.h>
#include <direct.h>

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
 * the user.
 * </p>
 */
struct term_data {
    term_type t{};
    concptr s{};
    HWND w{};
    DWORD dwStyle{};
    DWORD dwExStyle{};

    uint keys{};
    TERM_LEN rows{}; /* int -> uint */
    TERM_LEN cols{};

    uint pos_x{}; //!< タームの左上X座標
    uint pos_y{}; //!< タームの左上Y座標
    uint size_wid{};
    uint size_hgt{};
    uint size_ow1{};
    uint size_oh1{};
    uint size_ow2{};
    uint size_oh2{};

    bool size_hack{};
    bool xtra_hack{};
    bool visible{};
    concptr font_want{};
    HFONT font_id{};
    int font_wid{}; //!< フォント横幅
    int font_hgt{}; //!< フォント縦幅
    int tile_wid{}; //!< タイル横幅
    int tile_hgt{}; //!< タイル縦幅

    LOGFONT lf{};

    bool posfix{};

    HDC hdc{}; //!< ビットマップ描画コンテキスト
    std::vector<RECT> rc_buf{}; //!< 描画バッファサイズ

    term_data() = default; //!< デフォルトコンストラクタ
    term_data(const term_data &) = delete; //!< コピー禁止
    term_data &operator=(term_data &) = delete; //!< 代入演算子禁止
    term_data &operator=(term_data &&) = default; //!< move代入

    /*!
     * @brief バッファ用ビットマップ描画コンテキストを作成する
     */
    void create_hdc()
    {
        RECT rc;
        auto _hdc = GetDC(this->w);
        GetWindowRect(this->w, &rc);
        auto bmp = CreateCompatibleBitmap(_hdc, rc.right, rc.bottom);
        this->hdc = CreateCompatibleDC(_hdc);
        SelectObject(this->hdc, bmp);
        DeleteObject(bmp);
        ReleaseDC(this->w, _hdc);
    }

    /*!
     * @brief バッファビットマップをクリアする(リサイズでゴミを消すため)
     */
    void clear_hdc()
    {
        if (this->hdc == NULL)
            return;

        RECT rc;
        auto _hdc = GetDC(this->w);
        GetWindowRect(this->w, &rc);
        auto bmp = CreateCompatibleBitmap(_hdc, rc.right, rc.bottom);
        DeleteObject(SelectObject(this->hdc, bmp));
        ReleaseDC(this->w, _hdc);
        this->rc_buf.clear();
    }

    /*!
     * @brief 描画領域を追加
     * @param rc 描画領域
     */
    void add_rc(RECT& rc)
    {
        this->rc_buf.push_back(std::move(rc));
    }

    /*!
     * @brief 描画領域を全領域にする
     */
    void set_redraw()
    {
        this->rc_buf.clear();
    }

    /*!
     * @brief ウィンドウをバッファビットマップで再描画する
     */
    void update_hdc()
    {
        if (this->hdc == NULL)
            this->create_hdc();

        PAINTSTRUCT ps;
        auto _hdc = BeginPaint(this->w, &ps);
        if (this->rc_buf.size() == 0 || this->rc_buf.size() > 2)
            BitBlt(_hdc, 0, 0, ps.rcPaint.right, ps.rcPaint.bottom, this->hdc, 0, 0, SRCCOPY);
        else {
            for (RECT rc : this->rc_buf)
                BitBlt(_hdc, rc.left, rc.top, rc.right, rc.bottom, this->hdc, rc.left, rc.top, SRCCOPY);
        }
        EndPaint(this->w, &ps);
        this->rc_buf.clear();
    }
};

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

/* bg */
bg_mode current_bg_mode = bg_mode::BG_NONE;
#define DEFAULT_BG_FILENAME "bg.bmp"
char wallpaper_file[MAIN_WIN_MAX_PATH] = ""; //!< 壁紙ファイル名。

/*!
 * 現在使用中のタイルID(0ならば未使用)
 * Flag set once "graphics" has been initialized
 */
static byte current_graphics_mode = 0;

/*
 * The global tile
 */
static tile_info infGraph;

/*
 * The global tile mask
 */
static tile_info infMask;

/*
 * Show sub-windows even when Hengband is not in focus
 */
static bool keep_subwindows = TRUE;

/*
 * Redraw window during resize it
 */
static bool redraw_resize_window = TRUE;

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
    strcpy(buf, use_pause_music_inactive ? "1" : "0");
    WritePrivateProfileString("Angband", "MusicPauseInactive", buf, ini_file);

    sprintf(buf, "%d", current_bg_mode);
    WritePrivateProfileString("Angband", "BackGround", buf, ini_file);
    WritePrivateProfileString("Angband", "BackGroundBitmap", wallpaper_file[0] != '\0' ? wallpaper_file : DEFAULT_BG_FILENAME, ini_file);

    int path_length = strlen(ANGBAND_DIR) - 4; /* \libの4文字分を削除 */
    char tmp[1024] = "";
    strncat(tmp, ANGBAND_DIR, path_length);

    int n = strncmp(tmp, savefile, path_length);
    if (n == 0) {
        char relative_path[1024] = "";
        snprintf(relative_path, sizeof(relative_path), ".\\%s", (savefile + path_length));
        WritePrivateProfileString("Angband", "SaveFile", relative_path, ini_file);
    } else {
        WritePrivateProfileString("Angband", "SaveFile", savefile, ini_file);
    }

    strcpy(buf, keep_subwindows ? "1" : "0");
    WritePrivateProfileString("Angband", "KeepSubwindows", buf, ini_file);

    strcpy(buf, redraw_resize_window ? "1" : "0");
    WritePrivateProfileString("Angband", "RedrawResizeWindow", buf, ini_file);

    for (int i = 0; i < MAX_TERM_DATA; ++i) {
        save_prefs_aux(i);
    }
}

/*
 * callback for EnumDisplayMonitors API
 */
BOOL CALLBACK monitorenumproc([[maybe_unused]] HMONITOR hMon, [[maybe_unused]] HDC hdcMon, [[maybe_unused]] LPRECT lpMon, LPARAM dwDate)
{
    bool *result = (bool *)dwDate;
    *result = true;
    return FALSE;
}

/*
 * Load the "prefs" for a single term
 */
static void load_prefs_aux(int i)
{
    term_data *td = &data[i];
    GAME_TEXT sec_name[128];
    char tmp[1024];

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

    int posx = GetPrivateProfileInt(sec_name, "PositionX", 0, ini_file);
    int posy = GetPrivateProfileInt(sec_name, "PositionY", 0, ini_file);
    // 保存座標がモニタ内の領域にあるかチェック
    RECT rect = { posx, posy, posx + 128, posy + 128 };
    bool in_any_monitor = false;
    ::EnumDisplayMonitors(NULL, &rect, monitorenumproc, (LPARAM)&in_any_monitor);
    if (in_any_monitor) {
        // いずれかのモニタに表示可能、ウインドウ位置を復元
        td->pos_x = posx;
        td->pos_y = posy;
    }

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
    use_pause_music_inactive = (GetPrivateProfileInt("Angband", "MusicPauseInactive", 0, ini_file) != 0);
    current_bg_mode = static_cast<bg_mode>(GetPrivateProfileInt("Angband", "BackGround", 0, ini_file));
    GetPrivateProfileString("Angband", "BackGroundBitmap", DEFAULT_BG_FILENAME, wallpaper_file, 1023, ini_file);
    GetPrivateProfileString("Angband", "SaveFile", "", savefile, 1023, ini_file);

    int n = strncmp(".\\", savefile, 2);
    if (n == 0) {
        int path_length = strlen(ANGBAND_DIR) - 4; /* \libの4文字分を削除 */
        char tmp[1024] = "";
        strncat(tmp, ANGBAND_DIR, path_length);
        strncat(tmp, savefile + 2, strlen(savefile) - 2 + path_length);
        strncpy(savefile, tmp, strlen(tmp));
    }

    keep_subwindows = (GetPrivateProfileInt("Angband", "KeepSubwindows", 0, ini_file) != 0);
    redraw_resize_window = (GetPrivateProfileInt("Angband", "RedrawResizeWindow", 1, ini_file) != 0);

    for (int i = 0; i < MAX_TERM_DATA; ++i) {
        load_prefs_aux(i);
    }
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
    char buf[MAIN_WIN_MAX_PATH];
    BYTE wid, hgt, twid, thgt, ox, oy;
    concptr name;
    concptr name_mask = NULL;

    infGraph.delete_bitmap();
    infMask.delete_bitmap();

    if (arg_graphics == GRAPHICS_ADAM_BOLT) {
        wid = 16;
        hgt = 16;
        twid = 16;
        thgt = 16;
        ox = 0;
        oy = 0;
        name = "16X16.BMP";
        name_mask = "mask.bmp";

        ANGBAND_GRAF = "new";
    } else if (arg_graphics == GRAPHICS_HENGBAND) {
        wid = 32;
        hgt = 32;
        twid = 32;
        thgt = 32;
        ox = 0;
        oy = 0;
        name = "32X32.BMP";
        name_mask = "mask32.bmp";

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
    infGraph.hBitmap = read_graphic(buf);
    if (!infGraph.hBitmap) {
        plog_fmt(_("ビットマップ '%s' を読み込めません。", "Cannot read bitmap file '%s'"), name);
        return FALSE;
    }

    infGraph.CellWidth = wid;
    infGraph.CellHeight = hgt;
    infGraph.TileWidth = twid;
    infGraph.TileHeight = thgt;
    infGraph.OffsetX = ox;
    infGraph.OffsetY = oy;

    if (name_mask) {
        path_build(buf, sizeof(buf), ANGBAND_DIR_XTRA_GRAF, name_mask);
        infMask.hBitmap = read_graphic(buf);
        if (!infMask.hBitmap) {
            plog_fmt("Cannot read bitmap file '%s'", buf);
            return FALSE;
        }
    }

    current_graphics_mode = arg_graphics;
    return (current_graphics_mode != GRAPHICS_NONE);
}

/*
 * Initialize music
 */
static void init_music(void)
{
    // Flag set once "music" has been initialized
    static bool can_use_music = FALSE;

    if (!can_use_music) {
        main_win_music::load_music_prefs();
        can_use_music = TRUE;
    }
}

/*
 * Initialize sound
 */
static void init_sound(void)
{
    // Flag set once "sound" has been initialized
    static bool can_use_sound = FALSE;

    if (!can_use_sound) {
        load_sound_prefs();
        can_use_sound = TRUE;
    }
}

/*!
 * @brief Change sound mode
 * @param new_mode bool
 */
static void change_sound_mode(bool new_mode)
{
    use_sound = new_mode;
    if (use_sound) {
        init_sound();
    }
}

/*
 * Initialize background
 */
static void init_background(void)
{
    // Flag set once "background" has been initialized
    static bool can_use_background = FALSE;

    if (!can_use_background) {
        load_bg_prefs();
        can_use_background = TRUE;
    }
}

/*!
 * @brief Change background mode
 * @param new_mode bg_mode
 * @param show_error trueに設定した場合のみ、エラーダイアログを表示する
 * @param force_redraw trueの場合、モード変更に関わらずウインドウを再描画する
 * @retval true success
 * @retval false failed
 */
static bool change_bg_mode(bg_mode new_mode, bool show_error = false, bool force_redraw = false)
{
    bg_mode old_bg_mode = current_bg_mode;
    current_bg_mode = new_mode;
    if (current_bg_mode != bg_mode::BG_NONE) {
        init_background();
        if (!load_bg(wallpaper_file)) {
            current_bg_mode = bg_mode::BG_NONE;
            if (show_error)
                plog_fmt(_("壁紙用ファイル '%s' を読み込めません。", "Can't load the image file '%s'."), wallpaper_file);
        }
    } else {
        delete_bg();
    }

    const bool mode_changed = (current_bg_mode != old_bg_mode);
    if (mode_changed || force_redraw) {
        // 全ウインドウ再描画
        for (int i = 0; i < MAX_TERM_DATA; i++) {
            term_data *td = &data[i];
            if (td->visible) {
                td->set_redraw();
                InvalidateRect(td->w, NULL, FALSE);
            }
        }
    }

    return (current_bg_mode == new_mode);
}

/*!
 * @brief 描画イベントを処理する
 * @param td 描画イベントを処理するTerm
 */
static void term_process_paint(term_data *td)
{
    if (!td->w)
        return;

    MSG msg;
    if (PeekMessage(&msg, td->w, WM_PAINT, WM_PAINT, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

/*!
 * @brief Resize a window
 */
static void term_window_resize(term_data *td)
{
    if (!td->w)
        return;

    SetWindowPos(td->w, 0, 0, 0, td->size_wid, td->size_hgt, SWP_NOMOVE | SWP_NOZORDER);
    td->set_redraw();
    InvalidateRect(td->w, NULL, FALSE);
}

/*!
 * @brief Force the use of a new font for a term_data.
 * This function may be called before the "window" is ready.
 * This function returns zero only if everything succeeds.
 * @note that the "font name" must be capitalized!!!
 */
static errr term_force_font(term_data *td)
{
    if (td->font_id)
        DeleteObject(td->font_id);

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
        hfOld = static_cast<HFONT>(SelectObject(hdcDesktop, td->font_id));
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

    term_force_font(td);
    td->tile_wid = td->font_wid;
    td->tile_hgt = td->font_hgt;
    term_getsize(td);
    term_window_resize(td);
    td->set_redraw();
    InvalidateRect(td->w, NULL, FALSE);
}

/*
 * Allow the user to lock this window.
 */
static void term_window_pos(term_data *td, HWND hWnd)
{
    SetWindowPos(td->w, hWnd, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

/*
 * Hack -- redraw a term_data
 */
static void term_data_redraw(term_data *td)
{
    term_activate(&td->t);
    term_redraw();
    td->set_redraw();
    InvalidateRect(td->w, NULL, FALSE);
    term_activate(term_screen);
}

void term_inversed_area(HWND hWnd, int x, int y, int w, int h)
{
    term_data *td = (term_data *)GetWindowLong(hWnd, 0);
    int tx = td->size_ow1 + x * td->tile_wid;
    int ty = td->size_oh1 + y * td->tile_hgt;
    int tw = w * td->tile_wid - 1;
    int th = h * td->tile_hgt - 1;

    HDC hdc = td->hdc;
    HBRUSH myBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));

    PatBlt(hdc, tx, ty, tw, th, PATINVERT);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

    RECT rc{ tx, ty, tx + tw, ty + th };
    td->add_rc(rc);
    InvalidateRect(td->w, NULL, FALSE);
}

/*!
 * @brief //!< Windows版ユーザ設定項目実装部(実装必須) /Interact with the User
 */
static errr term_user_win(int n)
{
    (void)n;
    return 0;
}

static void refresh_color_table()
{
    for (int i = 0; i < 256; i++) {
        byte rv = angband_color_table[i][1];
        byte gv = angband_color_table[i][2];
        byte bv = angband_color_table[i][3];
        win_clr[i] = PALETTERGB(rv, gv, bv);
    }
}

/*
 * React to global changes
 */
static errr term_xtra_win_react(player_type *player_ptr)
{
    refresh_color_table();

    if (arg_graphics && !init_graphics()) {
        plog(_("グラフィックスを初期化できません!", "Cannot initialize graphics!"));
        arg_graphics = GRAPHICS_NONE;
    }
    use_graphics = (arg_graphics > 0);
    reset_visuals(player_ptr);

    for (int i = 0; i < MAX_TERM_DATA; i++) {
        term_type *old = Term;
        term_data *td = &data[i];
        if ((td->cols != td->t.wid) || (td->rows != td->t.hgt)) {
            term_activate(&td->t);
            term_resize(td->cols, td->rows);
            term_redraw();
            td->set_redraw();
            InvalidateRect(td->w, NULL, FALSE);
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

    HDC hdc = td->hdc;
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    if (current_bg_mode != bg_mode::BG_NONE) {
        rc.left = 0;
        rc.top = 0;
        draw_bg(hdc, &rc);
    }

    td->set_redraw();
    InvalidateRect(td->w, NULL, FALSE);
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
    if (!use_sound)
        return 1;
    return play_sound(v);
}

/*
 * Hack -- play a music
 */
static errr term_xtra_win_music(int n, int v)
{
    if (!use_music) {
        return 1;
    }

    return main_win_music::play_music(n, v);
}

/*
 * Hack -- play a music matches a situation
 */
static errr term_xtra_win_scene(int v)
{
    // TODO 場面に合った壁紙変更対応
    if (!use_music) {
        return 1;
    }

    return main_win_music::play_music_scene(v);
}

/*
 * Delay for "x" milliseconds
 */
static int term_xtra_win_delay(int v)
{
    auto *td = &data[0];
    InvalidateRect(td->w, NULL, FALSE);
    term_process_paint(td);
    Sleep(v);
    return 0;
}

/*!
 * @brief Do a "special thing"
 * @todo z-termに影響があるのでplayer_typeの追加は保留
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
    case TERM_XTRA_MUSIC_TOWN:
    case TERM_XTRA_MUSIC_MONSTER: {
        return term_xtra_win_music(n, v);
    }
    case TERM_XTRA_MUSIC_MUTE: {
        return main_win_music::stop_music();
    }
    case TERM_XTRA_SCENE: {
        return term_xtra_win_scene(v);
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
    tile_wid = td->tile_wid;
    tile_hgt = td->tile_hgt;

    RECT rc;
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = td->hdc;
    FrameRect(hdc, &rc, hbrYellow);
    td->add_rc(rc);
    InvalidateRect(td->w, NULL, FALSE);
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
    tile_wid = td->tile_wid;
    tile_hgt = td->tile_hgt;

    RECT rc;
    rc.left = x * tile_wid + td->size_ow1;
    rc.right = rc.left + 2 * tile_wid;
    rc.top = y * tile_hgt + td->size_oh1;
    rc.bottom = rc.top + tile_hgt;

    HDC hdc = td->hdc;
    FrameRect(hdc, &rc, hbrYellow);
    td->add_rc(rc);
    InvalidateRect(td->w, NULL, FALSE);
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

    HDC hdc = td->hdc;
    SetBkColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, td->font_id);
    if (current_bg_mode != bg_mode::BG_NONE)
        draw_bg(hdc, &rc);
    else
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    td->set_redraw();
    InvalidateRect(td->w, NULL, FALSE);
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

    RECT rc{ static_cast<LONG>(x * td->tile_wid + td->size_ow1), static_cast<LONG>(y * td->tile_hgt + td->size_oh1),
        static_cast<LONG>(rc.left + n * td->tile_wid), static_cast<LONG>(rc.top + td->tile_hgt) };

    HDC hdc = td->hdc;
    SetBkColor(hdc, RGB(0, 0, 0));
    SetTextColor(hdc, win_clr[a]);

    SelectObject(hdc, td->font_id);
    if (current_bg_mode != bg_mode::BG_NONE)
        SetBkMode(hdc, TRANSPARENT);

    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    if (current_bg_mode != bg_mode::BG_NONE)
        draw_bg(hdc, &rc);

    rc.left += ((td->tile_wid - td->font_wid) / 2);
    rc.right = rc.left + td->font_wid;
    rc.top += ((td->tile_hgt - td->font_hgt) / 2);
    rc.bottom = rc.top + td->font_hgt;
    RECT rc_start = rc;

    for (int i = 0; i < n; i++) {
#ifdef JP
        if (use_bigtile && *(s + i) == "■"[0] && *(s + i + 1) == "■"[1]) {
            rc.right += td->font_wid;
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
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
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
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
            oldBrush = static_cast<HBRUSH>(SelectObject(hdc, myBrush));
            oldPen = static_cast<HPEN>(SelectObject(hdc, GetStockObject(NULL_PEN)));
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

    rc.left = rc_start.left;
    rc.top = rc_start.top;
    td->add_rc(rc);
    InvalidateRect(td->w, NULL, FALSE);
    if (!initialized)
        term_process_paint(td);
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
    w2 = td->tile_wid;
    h2 = td->tile_hgt;
    tw2 = w2;
    if (use_bigtile)
        tw2 *= 2;

    TERM_LEN x2 = x * w2 + td->size_ow1 + infGraph.OffsetX;
    TERM_LEN y2 = y * h2 + td->size_oh1 + infGraph.OffsetY;
    HDC hdc = td->hdc;
    HDC hdcSrc = CreateCompatibleDC(hdc);
    HBITMAP hbmSrcOld = static_cast<HBITMAP>(SelectObject(hdcSrc, infGraph.hBitmap));

    if (arg_graphics == GRAPHICS_ADAM_BOLT || arg_graphics == GRAPHICS_HENGBAND) {
        hdcMask = CreateCompatibleDC(hdc);
        SelectObject(hdcMask, infMask.hBitmap);
    }

    RECT rc;
    rc.left = x2;
    rc.top = y2;
    rc.bottom = h2 * h1 / th1 - 1;

    for (i = 0; i < n; i++, x2 += w2) {
        TERM_COLOR a = ap[i];
        char c = cp[i];
        int row = (a & 0x7F);
        int col = (c & 0x7F);
        TERM_LEN x1 = col * w1;
        TERM_LEN y1 = row * h1;
        rc.left += w2;

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

    td->add_rc(rc);
    InvalidateRect(td->w, NULL, FALSE);
    return 0;
}

/*
 * Create and initialize a "term_data" given a title
 */
static void term_data_link(term_data *td)
{
    term_type *t = &td->t;
    term_init(t, td->cols, td->rows, FILE_READ_BUFF_SIZE);
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
        term_force_font(td);
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
        td->w
            = CreateWindowEx(td->dwExStyle, AngList, td->s, td->dwStyle, td->pos_x, td->pos_y, td->size_wid, td->size_hgt, HWND_DESKTOP, NULL, hInstance, NULL);
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

        td->create_hdc();
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
    hbrYellow = CreateSolidBrush(win_clr[TERM_YELLOW]);
    td->create_hdc();
    (void)term_xtra_win_flush();
}

/*
 * Prepare the menus
 */
static void setup_menus(void)
{
    HMENU hm = GetMenu(data[0].w);

    if (current_world_ptr->character_generated) {
        EnableMenuItem(hm, IDM_FILE_NEW, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        EnableMenuItem(hm, IDM_FILE_OPEN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        EnableMenuItem(hm, IDM_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED);
    } else {
        EnableMenuItem(hm, IDM_FILE_NEW, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hm, IDM_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
        EnableMenuItem(hm, IDM_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    }

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
    CheckMenuItem(hm, IDM_WINDOW_KEEP_SUBWINDOWS, (keep_subwindows ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_WINDOW_REDRAW_RESIZEING, (redraw_resize_window ? MF_CHECKED : MF_UNCHECKED));

    CheckMenuItem(hm, IDM_OPTIONS_NO_GRAPHICS, (arg_graphics == GRAPHICS_NONE ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_OLD_GRAPHICS, (arg_graphics == GRAPHICS_ORIGINAL ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_NEW_GRAPHICS, (arg_graphics == GRAPHICS_ADAM_BOLT ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_NEW2_GRAPHICS, (arg_graphics == GRAPHICS_HENGBAND ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_BIGTILE, (arg_bigtile ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_MUSIC, (arg_music ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_MUSIC_PAUSE_INACTIVE, (use_pause_music_inactive ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_SOUND, (arg_sound ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_NO_BG, ((current_bg_mode == bg_mode::BG_NONE) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_BG, ((current_bg_mode == bg_mode::BG_ONE) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hm, IDM_OPTIONS_PRESET_BG, ((current_bg_mode == bg_mode::BG_PRESET) ? MF_CHECKED : MF_UNCHECKED));
    // TODO IDM_OPTIONS_PRESET_BG を有効にする
    EnableMenuItem(hm, IDM_OPTIONS_PRESET_BG, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
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

    strcpy(savefile, s);
    validate_file(savefile);
    game_in_progress = TRUE;
    play_game(player_ptr, FALSE, FALSE);
}

/*
 * Process a menu command
 */
static void process_menus(player_type *player_ptr, WORD wCmd)
{
    if (!initialized) {
        plog(_("まだ初期化中です...", "You cannot do that yet..."));
        return;
    }

    term_data *td;
    OPENFILENAME ofn;
    switch (wCmd) {
    case IDM_FILE_NEW: {
        if (game_in_progress) {
            plog(_("プレイ中は新しいゲームを始めることができません！", "You can't start a new game while you're still playing!"));
        } else {
            game_in_progress = TRUE;
            term_flush();
            strcpy(savefile, "");
            play_game(player_ptr, TRUE, FALSE);
            quit(NULL);
        }

        break;
    }
    case IDM_FILE_OPEN: {
        if (game_in_progress) {
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
        if (game_in_progress) {
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
            term_data_redraw(td);
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
    case IDM_WINDOW_KEEP_SUBWINDOWS: {
        keep_subwindows = !keep_subwindows;
        break;
    }
    case IDM_WINDOW_REDRAW_RESIZEING: {
        redraw_resize_window = !redraw_resize_window;
        break;
    }
    case IDM_OPTIONS_NO_GRAPHICS: {
        if (arg_graphics != GRAPHICS_NONE) {
            arg_graphics = GRAPHICS_NONE;
            if (game_in_progress)
                do_cmd_redraw(player_ptr);
        }
        break;
    }
    case IDM_OPTIONS_OLD_GRAPHICS: {
        if (arg_graphics != GRAPHICS_ORIGINAL) {
            arg_graphics = GRAPHICS_ORIGINAL;
            if (game_in_progress)
                do_cmd_redraw(player_ptr);
        }

        break;
    }
    case IDM_OPTIONS_NEW_GRAPHICS: {
        if (arg_graphics != GRAPHICS_ADAM_BOLT) {
            arg_graphics = GRAPHICS_ADAM_BOLT;
            if (game_in_progress)
                do_cmd_redraw(player_ptr);
        }

        break;
    }
    case IDM_OPTIONS_NEW2_GRAPHICS: {
        if (arg_graphics != GRAPHICS_HENGBAND) {
            arg_graphics = GRAPHICS_HENGBAND;
            if (game_in_progress)
                do_cmd_redraw(player_ptr);
        }

        break;
    }
    case IDM_OPTIONS_BIGTILE: {
        td = &data[0];
        arg_bigtile = !arg_bigtile;
        term_activate(&td->t);
        term_resize(td->cols, td->rows);
        td->set_redraw();
        InvalidateRect(td->w, NULL, FALSE);
        break;
    }
    case IDM_OPTIONS_MUSIC: {
        arg_music = !arg_music;
        use_music = arg_music;
        if (use_music) {
            init_music();
            if (game_in_progress)
                select_floor_music(player_ptr);
        } else {
            main_win_music::stop_music();
        }
        break;
    }
    case IDM_OPTIONS_MUSIC_PAUSE_INACTIVE: {
        use_pause_music_inactive = !use_pause_music_inactive;
        break;
    }
    case IDM_OPTIONS_OPEN_MUSIC_DIR: {
        std::vector<char> buf(MAIN_WIN_MAX_PATH);
        path_build(&buf[0], MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_MUSIC, "music.cfg");
        open_dir_in_explorer(&buf[0]);
        break;
    }
    case IDM_OPTIONS_SOUND: {
        arg_sound = !arg_sound;
        change_sound_mode(arg_sound);
        break;
    }
    case IDM_OPTIONS_OPEN_SOUND_DIR: {
        std::vector<char> buf(MAIN_WIN_MAX_PATH);
        path_build(&buf[0], MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
        open_dir_in_explorer(&buf[0]);
        break;
    }
    case IDM_OPTIONS_NO_BG: {
        change_bg_mode(bg_mode::BG_NONE);
        td = &data[0];
        term_redraw();
        td->set_redraw();
        InvalidateRect(td->w, NULL, FALSE);
        break;
    }
    case IDM_OPTIONS_PRESET_BG: {
        change_bg_mode(bg_mode::BG_PRESET);
        td = &data[0];
        term_redraw();
        td->set_redraw();
        InvalidateRect(td->w, NULL, FALSE);
        break;
    }
    case IDM_OPTIONS_BG: {
        bool ret = change_bg_mode(bg_mode::BG_ONE);
        if (ret) {
            td = &data[0];
            term_redraw();
            td->set_redraw();
            InvalidateRect(td->w, NULL, FALSE);
            break;
        }
        // 壁紙の設定に失敗した（ファイルが存在しない等）場合、壁紙に使うファイルを選択させる
    }
        [[fallthrough]]; /* Fall through */
    case IDM_OPTIONS_OPEN_BG: {
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = data[0].w;
        ofn.lpstrFilter = "Image Files (*.bmp;*.png;*.jpg;*.jpeg;)\0*.bmp;*.png;*.jpg;*.jpeg;\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = wallpaper_file;
        ofn.nMaxFile = 1023;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = _("壁紙を選んでね。", "Choose wall paper.");
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

        if (GetOpenFileName(&ofn)) {
            change_bg_mode(bg_mode::BG_ONE, true, true);
            td = &data[0];
            term_redraw();
            td->set_redraw();
            InvalidateRect(td->w, NULL, FALSE);
        }
        break;
    }
    case IDM_DUMP_SCREEN_HTML: {
        save_screen_as_html(data[0].w);
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
            [[fallthrough]]; /* Fall through */
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
            [[fallthrough]]; /* Fall through */
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

static void handle_app_active(HWND hWnd, UINT uMsg, WPARAM wParam, [[maybe_unused]] LPARAM lParam)
{
    switch (uMsg) {
    case WM_ACTIVATEAPP: {
        if (wParam) {
            if (use_pause_music_inactive)
                main_win_music::resume_music();
        } else {
            if (use_pause_music_inactive)
                main_win_music::pause_music();
        }
        break;
    }
    case WM_WINDOWPOSCHANGING: {
        if (!IsIconic(hWnd))
            if (use_pause_music_inactive)
                main_win_music::resume_music();
        break;
    }
    }
}

/*!
 * @brief メインウインドウ用ウインドウプロシージャ
 */
LRESULT PASCAL AngbandWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    term_data *td = (term_data *)GetWindowLong(hWnd, 0);

    handle_app_active(hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_NCCREATE: {
        SetWindowLong(hWnd, 0, (LONG)(my_td));
        break;
    }
    case WM_CREATE: {
        setup_mci(hWnd);
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
    case WM_ERASEBKGND: {
        return 1;
    }
    case WM_PAINT: {
        td->update_hdc();
        ValidateRect(hWnd, NULL);
        return 0;
    }
    case MM_MCINOTIFY: {
        main_win_music::on_mci_notify(wParam, lParam);

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
            const auto &scr = data[0].t.scr->c;

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
        InvalidateRect(td->w, NULL, FALSE);
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
        InvalidateRect(td->w, NULL, FALSE);
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
        (void)save_player(p_ptr, SAVE_TYPE_CLOSE_GAME);
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

        //!< @todo 二重のswitch文。後で分割する.
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
                if (wParam == SIZE_MAXIMIZED || redraw_resize_window) {
                    term_resize(td->cols, td->rows);
                    td->clear_hdc();
                    term_redraw();
                }
                InvalidateRect(td->w, NULL, FALSE);
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
    case WM_EXITSIZEMOVE: {
        term_resize(td->cols, td->rows);
        td->clear_hdc();
        term_redraw();
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
        [[fallthrough]]; /* Fall through */
    case WM_ENABLE: {
        if (wParam == FALSE && keep_subwindows) {
            for (int i = 0; i < MAX_TERM_DATA; i++) {
                if (data[i].visible) {
                    ShowWindow(data[i].w, SW_SHOW);
                }
            }
        }
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*!
 * @brief サブウインドウ用ウインドウプロシージャ
 */
LRESULT PASCAL AngbandListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    term_data *td = (term_data *)GetWindowLong(hWnd, 0);

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
            if (wParam == SIZE_MAXIMIZED || redraw_resize_window) {
                term_resize(td->cols, td->rows);
                td->clear_hdc();
                term_redraw();
                term_activate(old_term);
                p_ptr->window_flags = PW_ALL;
                handle_stuff(p_ptr);
            }
            InvalidateRect(td->w, NULL, FALSE);
        }

        td->size_hack = FALSE;
        return 0;
    }
    case WM_EXITSIZEMOVE: {
        term_type *old_term = Term;
        term_activate(&td->t);
        term_resize(td->cols, td->rows);
        td->clear_hdc();
        term_redraw();
        term_activate(old_term);
        p_ptr->window_flags = PW_ALL;
        handle_stuff(p_ptr);
        return 0;
    }
    case WM_ERASEBKGND: {
        return 1;
    }
    case WM_PAINT: {
        td->update_hdc();
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

/*
 * Display warning message (see "z-util.c")
 */
static void hack_plog(concptr str)
{
    if (str) {
        MessageBoxW(NULL, to_wchar(str).wc_str(), _(L"警告！", L"Warning"), MB_ICONEXCLAMATION | MB_OK);
    }
}

/*
 * Display error message and quit (see "z-util.c")
 */
static void hack_quit(concptr str)
{
    if (str) {
        MessageBoxW(NULL, to_wchar(str).wc_str(), _(L"エラー！", L"Error"), MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
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
        MessageBoxW(data[0].w, to_wchar(str).wc_str(), _(L"警告！", L"Warning"), MB_ICONEXCLAMATION | MB_OK);
    }
}

/*
 * Display error message and quit (see "z-util.c")
 */
static void hook_quit(concptr str)
{
    if (str) {
        MessageBoxW(data[0].w, to_wchar(str).wc_str(), _(L"エラー！", L"Error"), MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
    }

    save_prefs();
    for (int i = MAX_TERM_DATA - 1; i >= 0; --i) {
        term_force_font(&data[i]);
        if (data[i].font_want)
            string_free(data[i].font_want);
        if (data[i].w)
            DestroyWindow(data[i].w);
        data[i].w = 0;
    }

    infGraph.delete_bitmap();
    infMask.delete_bitmap();

    DeleteObject(hbrYellow);
    finalize_bg();
    finalize_graphics();

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
    validate_dir(ANGBAND_DIR_DEBUG_SAVE, FALSE);
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
}

/*!
 * @brief コマンドラインから全スポイラー出力を行う
 * Create Spoiler files from Command Line
 * @return spoiler_output_status
 */
static spoiler_output_status create_debug_spoiler(LPSTR cmd_line)
{
    char *s;
    concptr option;
    s = cmd_line;
    if (!*s)
        return SPOILER_OUTPUT_CANCEL;
    option = "--output-spoilers";

    if (strncmp(s, option, strlen(option)) != 0)
        return SPOILER_OUTPUT_CANCEL;

    init_stuff();
    init_angband(p_ptr, TRUE);

    return output_all_spoilers();
}

/*!
 * @brief (Windows固有)Windowsアプリケーションとしてのエントリポイント
 */
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPSTR lpCmdLine, [[maybe_unused]] _In_ int nCmdShow)
{
    setlocale(LC_ALL, "ja_JP");
    hInstance = hInst;
    if (is_already_running()) {
        MessageBoxW(
            NULL, _(L"変愚蛮怒はすでに起動しています。", L"Hengband is already running."), _(L"エラー！", L"Error"), MB_ICONEXCLAMATION | MB_OK | MB_ICONSTOP);
        return FALSE;
    }

    switch (create_debug_spoiler(lpCmdLine)) {
    case SPOILER_OUTPUT_SUCCESS:
        fprintf(stdout, "Successfully created a spoiler file.");
        quit(NULL);
        return 0;
    case SPOILER_OUTPUT_FAIL_FOPEN:
        fprintf(stderr, "Cannot create spoiler file.");
        quit(NULL);
        return 0;
    case SPOILER_OUTPUT_FAIL_FCLOSE:
        fprintf(stderr, "Cannot close spoiler file.");
        quit(NULL);
        return 0;
    default:
        break;
    }

    if (hPrevInst == NULL) {
        WNDCLASS wc;
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = AngbandWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 4;
        wc.hInstance = hInst;
        wc.hIcon = hIcon = LoadIcon(hInst, AppName);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName = AppName;
        wc.lpszClassName = AppName;

        if (!RegisterClass(&wc))
            exit(1);

        wc.lpfnWndProc = AngbandListProc;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = AngList;

        if (!RegisterClass(&wc))
            exit(2);
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

    HDC hdc = GetDC(NULL);
    if (GetDeviceCaps(hdc, BITSPIXEL) <= 8) {
        quit(_("画面を16ビット以上のカラーモードにして下さい。", "Please switch to High Color (16-bit) or higher color mode."));
    }
    ReleaseDC(NULL, hdc);

    refresh_color_table();
    init_windows();
    change_bg_mode(current_bg_mode, true);

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
    init_angband(p_ptr, FALSE);
    initialized = TRUE;
    check_for_save_file(p_ptr, lpCmdLine);
    prt(_("[ファイル] メニューの [新規] または [開く] を選択してください。", "[Choose 'New' or 'Open' from the 'File' menu]"), 23, _(8, 17));
    term_fresh();

    change_sound_mode(arg_sound);
    use_music = arg_music;
    if (use_music) {
        init_music();
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    quit(NULL);
    return 0;
}
#endif /* WINDOWS */
