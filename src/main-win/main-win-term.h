#pragma once
/*!
 * @file main-win-term.h
 * @brief Windows版固有実装(ターミナル)ヘッダ
 */

#include "main-win/graphics-win.h"
#include "main-win/main-win-bg.h"
#include "system/h-type.h"
#include "term/z-term.h"
#include <windows.h>

/*!
 * @brief ダブルバッファリング
 */
class double_buffering {
public:
    double_buffering()
        : display(NULL)
        , bitmap(NULL)
        , offscreen(NULL)
    {
    }

    HDC get_hdc(HWND);
    bool render(const RECT &rect);
    void dispose_offscreen();

protected:
    HDC display;
    HBITMAP bitmap;
    HDC offscreen;
};

constexpr LPCWSTR application_name = L"ANGBAND";
constexpr int MAX_TERM_DATA = 8; //!< Maximum number of windows
extern HINSTANCE hInstance; // Saved instance handle.
extern bg_mode current_bg_mode;
extern COLORREF win_clr[256]; // The "complex" color values.
extern POINT normsize; //!< Remember normal size of main window when maxmized.
extern HBRUSH hbrYellow; // Yellow brush for the cursor.

struct term_data;
extern term_data data[MAX_TERM_DATA]; //!< An array of term_data's

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
struct player_type;
struct term_data {
    term_data() = default; //!< デフォルトコンストラクタ
    term_data(const term_data &) = delete; //!< コピー禁止
    term_data &operator=(term_data &) = delete; //!< 代入演算子禁止
    term_data &operator=(term_data &&) = default; //!< move代入

    term_type t{};
    LPCWSTR name{};
    HWND w{};
    DWORD dwStyle{};
    DWORD dwExStyle{};

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
    bool visible{};
    concptr font_want{};
    HFONT font_id{};
    int font_wid{}; //!< フォント横幅
    int font_hgt{}; //!< フォント縦幅
    int tile_wid{}; //!< タイル横幅
    int tile_hgt{}; //!< タイル縦幅

    LOGFONTW lf{};
    bool posfix{};
    double_buffering graphics{};

    static errr extra_win_flush();
    static void refresh_color_table();
    static void change_graphics_mode(graphics_mode mode);

    HDC get_hdc();
    void refresh(const RECT *lpRect = NULL);
    bool render(const RECT &rect);
    void resize_window();
    void adjust_size();
    void force_font();
    void change_font();
    void set_window_position(HWND hWnd);
    void redraw_data();
    void rebuild(bool resize_window = true);
    void fit_size_to_window(bool recalc_window_size = false);
    void link_data();
    bool handle_window_resize(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static errr user_win(int n);
    static errr extra_win(int n, int v);
    static errr curs_win(int x, int y);
    static errr bigcurs_win(int x, int y);
    static errr wipe_win(int x, int y, int n);
    static errr text_win(int x, int y, int n, TERM_COLOR a, concptr s);
    static errr pict_win(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, concptr cp, const TERM_COLOR *tap, concptr tcp);
    static errr extra_win_react(player_type *player_ptr);
    static errr extra_win_event(int v);
    static errr extra_win_clear(void);
    static errr extra_win_noise(void);
    static errr extra_win_sound(int v);
    static errr extra_win_music(int n, int v);
    static errr extra_win_scene(int v);
    static int extra_win_delay(int v);

    void dispose_offscreen();
    bool is_main_term();
    bool handle_window_get_info(const LPARAM lParam);
    bool handle_window_position_change(const LPARAM lParam);
    bool handle_window_max_min(const WPARAM wParam);
};
