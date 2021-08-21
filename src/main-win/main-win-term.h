﻿#pragma once
/*!
 * @file main-win-term.h
 * @brief Windows版固有実装(ターミナル)ヘッダ
 */

#include "term/z-term.h"
#include "system/h-type.h"

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

    term_data() = default; //!< デフォルトコンストラクタ
    term_data(const term_data &) = delete; //!< コピー禁止
    term_data &operator=(term_data &) = delete; //!< 代入演算子禁止
    term_data &operator=(term_data &&) = default; //!< move代入

    double_buffering graphics{};
    HDC get_hdc();
    void refresh(const RECT *lpRect = NULL);
    bool render(const RECT &rect);
    void dispose_offscreen();
    bool is_main_term();
    void adjust_size();
};

constexpr int MAX_TERM_DATA = 8; //!< Maximum number of windows
extern term_data data[MAX_TERM_DATA]; //!< An array of term_data's
