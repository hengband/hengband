#pragma once

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#include "system/angband.h"
#include "system/h-basic.h"
#include <memory>
#include <optional>
#include <stack>
#include <string_view>
#include <utility>
#include <vector>

/*!
 * @brief A term_win is a "window" for a Term
 */
class term_win {
public:
    static std::unique_ptr<term_win> create(TERM_LEN w, TERM_LEN h);
    std::unique_ptr<term_win> clone() const;
    void resize(TERM_LEN w, TERM_LEN h);

    bool cu{}, cv{}; //!< Cursor Useless / Visible codes
    TERM_LEN cx{}, cy{}; //!< Cursor Location (see "Useless")

    std::vector<std::vector<TERM_COLOR>> a; //!< Array[h*w] -- Attribute array
    std::vector<std::vector<char>> c; //!< Array[h*w] -- Character array

    std::vector<std::vector<TERM_COLOR>> ta; //!< Note that the attr pair at(x, y) is a[y][x]
    std::vector<std::vector<char>> tc; //!< Note that the char pair at(x, y) is c[y][x]

private:
    term_win(TERM_LEN w, TERM_LEN h);
};

/*!
 * @brief term実装構造体 / An actual "term" structure
 */
struct term_type {
    vptr user{}; //!< Extra "user" info (used by application)
    vptr data{}; //!< Extra "data" info (used by implementation)

    bool user_flag{}; //!< Flag "user_flag" An extra "user" flag (used by application)
    bool data_flag{}; //!< Flag "data_flag" An extra "data" flag (used by implementation)

    bool active_flag{}; //!< Flag "active_flag" This "term" is "active"
    bool mapped_flag{}; //!< Flag "mapped_flag" This "term" is "mapped"
    bool total_erase{}; //!< Flag "total_erase" This "term" should be fully erased
    bool fixed_shape{}; //!< Flag "fixed_shape" This "term" is not allowed to resize
    bool icky_corner{}; //!< Flag "icky_corner" This "term" has an "icky" corner grid
    bool soft_cursor{}; //!< Flag "soft_cursor" This "term" uses a "software" cursor
    bool always_pict{}; //!< Flag "always_pict" Use the "Term_pict()" routine for all text
    bool higher_pict{}; //!< Flag "higher_pict" Use the "Term_pict()" routine for special text
    bool always_text{}; //!< Flag "always_text" Use the "Term_text()" routine for invisible text
    bool unused_flag{}; //!< Flag "unused_flag" Reserved for future use
    bool never_bored{}; //!< Flag "never_bored" Never call the "TERM_XTRA_BORED" action
    bool never_frosh{}; //!< Flag "never_frosh" Never call the "TERM_XTRA_FROSH" action
    bool never_fresh{}; //!< Flag "never_fresh" Never redraw the Term

    byte attr_blank{}; //!< Value "attr_blank" Use this "attr" value for "blank" grids
    char char_blank{}; //!< Value "char_blank" Use this "char" value for "blank" grids

    std::vector<char> key_queue; //!< Keypress Queue -- various data / Keypress Queue -- pending keys
    uint16_t key_head{};
    uint16_t key_tail{};
    uint16_t key_xtra{};
    uint16_t key_size{};

    TERM_LEN wid{}; //!< Window Width(max 255)
    TERM_LEN hgt{}; //!< Window Height(max 255)

    std::optional<TERM_LEN> centered_wid{};
    std::optional<TERM_LEN> centered_hgt{};

    TERM_LEN offset_x{};
    TERM_LEN offset_y{};

    TERM_LEN y1{}; //!< Minimum modified row
    TERM_LEN y2{}; //!< Maximum modified row

    std::vector<TERM_LEN> x1; //!< Minimum modified column(per row)
    std::vector<TERM_LEN> x2; //!< Maximum modified column(per row)

    std::unique_ptr<term_win> old; //!< Displayed screen image
    std::unique_ptr<term_win> scr; //!< Requested screen image

    std::unique_ptr<term_win> tmp; //!< Temporary screen image
    std::stack<std::unique_ptr<term_win>> mem_stack; //!< Memorized screen image stack

    void (*init_hook)(term_type *t){}; //!< Hook for init - ing the term
    void (*nuke_hook)(term_type *t){}; //!< Hook for nuke - ing the term

    errr (*user_hook)(int n){}; //!< ユーザ設定項目実装部 / Hook for user actions
    errr (*xtra_hook)(int n, int v){}; //!< 拡張機能実装部 / Hook for extra actions
    errr (*curs_hook)(TERM_LEN x, TERM_LEN y){}; //!< カーソル描画実装部 / Hook for placing the cursor
    errr (*bigcurs_hook)(TERM_LEN x, TERM_LEN y){}; //!< 大型タイル時カーソル描画実装部 / Hook for placing the cursor on bigtile mode
    errr (*wipe_hook)(TERM_LEN x, TERM_LEN y, int n){}; //!< 指定座標テキスト消去実装部 / Hook for drawing some blank spaces
    errr (*text_hook)(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, concptr s){}; //!< テキスト描画実装部 / Hook for drawing a string of chars using an attr
    void (*resize_hook)(void){}; //!< 画面リサイズ実装部
    errr (*pict_hook)(TERM_LEN x, TERM_LEN y, int n, const TERM_COLOR *ap, concptr cp, const TERM_COLOR *tap,
        concptr tcp){}; //!< タイル描画実装部 / Hook for drawing a sequence of special attr / char pairs

    // std::unique_ptr をメンバに持つため、コピーはできない。
    // コピーコンストラクタ/コピー代入演算子が暗黙に削除されたという
    // MSVCの警告を避けるためコンストラクタ等を明示的に定義する。
    term_type() = default;
    ~term_type() = default;
    term_type(const term_type &) = delete;
    term_type &operator=(const term_type &) = delete;
    term_type(term_type &&) = default;
    term_type &operator=(term_type &&) = default;
};

class TermOffsetSetter {
public:
    TermOffsetSetter(std::optional<TERM_LEN> x, std::optional<TERM_LEN> y);
    ~TermOffsetSetter();
    TermOffsetSetter(const TermOffsetSetter &) = delete;
    TermOffsetSetter &operator=(const TermOffsetSetter &) = delete;
    TermOffsetSetter(TermOffsetSetter &&) = delete;
    TermOffsetSetter &operator=(TermOffsetSetter &&) = delete;

private:
    term_type *term;
    TERM_LEN orig_offset_x;
    TERM_LEN orig_offset_y;
};

class TermCenteredOffsetSetter {
public:
    TermCenteredOffsetSetter(std::optional<TERM_LEN> width, std::optional<TERM_LEN> height);
    ~TermCenteredOffsetSetter();
    TermCenteredOffsetSetter(const TermCenteredOffsetSetter &) = delete;
    TermCenteredOffsetSetter &operator=(const TermCenteredOffsetSetter &) = delete;
    TermCenteredOffsetSetter(TermCenteredOffsetSetter &&) = delete;
    TermCenteredOffsetSetter &operator=(TermCenteredOffsetSetter &&) = delete;

private:
    std::optional<TermOffsetSetter> tos;
    term_type *term;
    std::optional<TERM_LEN> orig_centered_wid;
    std::optional<TERM_LEN> orig_centered_hgt;
};

/**** Available Constants ****/

/*
 * Definitions for the "actions" of "term_xtra()"
 *
 * These values may be used as the first parameter of "term_xtra()",
 * with the second parameter depending on the "action" itself.  Many
 * of the actions shown below are optional on at least one platform.
 *
 * The "TERM_XTRA_EVENT" action uses "v" to "wait" for an event
 * The "TERM_XTRA_SHAPE" action uses "v" to "show" the cursor
 * The "TERM_XTRA_FROSH" action uses "v" for the index of the row
 * The "TERM_XTRA_SOUND" action uses "v" for the index of a sound
 * The "TERM_XTRA_ALIVE" action uses "v" to "activate" (or "close")
 * The "TERM_XTRA_LEVEL" action uses "v" to "resume" (or "suspend")
 * The "TERM_XTRA_DELAY" action uses "v" as a "millisecond" value
 *
 * The other actions do not need a "v" code, so "zero" is used.
 */
#define TERM_XTRA_EVENT 1 /* Process some pending events */
#define TERM_XTRA_FLUSH 2 /* Flush all pending events */
#define TERM_XTRA_CLEAR 3 /* Clear the entire window */
#define TERM_XTRA_SHAPE 4 /* Set cursor shape (optional) */
#define TERM_XTRA_FROSH 5 /* Flush one row (optional) */
#define TERM_XTRA_FRESH 6 /* Flush all rows (optional) */
#define TERM_XTRA_NOISE 7 /* Make a noise (optional) */
#define TERM_XTRA_SOUND 8 /* Make a sound (optional) */
#define TERM_XTRA_BORED 9 /* Handle stuff when bored (optional) */
#define TERM_XTRA_REACT 10 /* React to global changes (optional) */
#define TERM_XTRA_ALIVE 11 /* Change the "hard" level (optional) */
#define TERM_XTRA_LEVEL 12 /* Change the "soft" level (optional) */
#define TERM_XTRA_DELAY 13 /* Delay some milliseconds (optional) */
#define TERM_XTRA_MUSIC_BASIC 14 /* Play a music(basic) (optional) */
#define TERM_XTRA_MUSIC_DUNGEON 15 /* Play a music(dungeon) (optional) */
#define TERM_XTRA_MUSIC_QUEST 16 /* Play a music(quest) (optional) */
#define TERM_XTRA_MUSIC_TOWN 17 /* Play a music(floor) (optional) */
#define TERM_XTRA_MUSIC_MONSTER 18 /* Play a music(monster) (optional) */
#define TERM_XTRA_MUSIC_MUTE 19
#define TERM_XTRA_SCENE 20 /* React to scene changes (optional) */

/**** Available Variables ****/
extern term_type *game_term;

errr term_user(int n);
errr term_xtra(int n, int v);

void term_queue_char(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c, TERM_COLOR ta, char tc);
void term_queue_bigchar(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c, TERM_COLOR ta, char tc);
void term_queue_line(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR *a, char *c, TERM_COLOR *ta, char *tc);

errr term_fresh(void);
errr term_fresh_force(void);
errr term_set_cursor(int v);
errr term_gotoxy(TERM_LEN x, TERM_LEN y);
errr term_draw(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c);
errr term_addch(TERM_COLOR a, char c);
errr term_add_bigch(TERM_COLOR a, char c);
errr term_addstr(int n, TERM_COLOR a, std::string_view sv);
errr term_putch(TERM_LEN x, TERM_LEN y, TERM_COLOR a, char c);
errr term_putstr(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, std::string_view sv);
errr term_erase(TERM_LEN x, TERM_LEN y, std::optional<int> n_opt = std::nullopt);
errr term_clear(void);
errr term_redraw(void);
errr term_redraw_section(TERM_LEN x1, TERM_LEN y1, TERM_LEN x2, TERM_LEN y2);

errr term_get_cursor(int *v);
std::pair<int, int> term_get_size();
errr term_locate(TERM_LEN *x, TERM_LEN *y);
errr term_what(TERM_LEN x, TERM_LEN y, TERM_COLOR *a, char *c);

errr term_flush(void);
errr term_key_push(int k);
errr term_inkey(char *ch, bool wait, bool take);

errr term_save(void);
errr term_load(bool load_all);

errr term_exchange(void);

errr term_resize(TERM_LEN w, TERM_LEN h);

errr term_activate(term_type *t);

errr term_init(term_type *t, TERM_LEN w, TERM_LEN h, int k);

#ifdef JP
errr term_putstr_v(TERM_LEN x, TERM_LEN y, int n, byte a, concptr s);
#endif

#ifndef WINDOWS
errr term_nuke(term_type *t);
#endif
