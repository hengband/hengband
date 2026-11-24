/*!
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
#include "locale/multibyte-char.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "term/tile-layout.h"
#include "view/display-symbol.h"
#include <algorithm>
#include <string>

//! Special flags in the attr data
constexpr auto AF_BIGTILE2 = 0xf0;
constexpr auto AF_TILE1 = 0x80;

/*!
 * @brief マスがタイル (属性と文字の最上位ビットが立っている) かを返す
 * @param a マスの属性
 * @param ch マスの文字
 * @return タイルなら true。全角文字は属性の最上位ビットが立っていてもタイルとしない
 */
static bool is_tile_cell(TERM_COLOR a, const TermChar &ch)
{
    return (a & AF_TILE1) && !ch.is_wide_lead() && (ch.front() & 0x80);
}

//! The current "term"
term_type *game_term = nullptr;
static const term_win *pict_source = nullptr;

const term_win &term_pict_buffer()
{
    return pict_source ? *pict_source : *game_term->scr;
}

TermTileBlock term_tile_block_at(const term_win &screen, int x, int y)
{
    TermTileBlock normal{ x, y, use_bigtile ? 2 : 1, 1 };
    const auto height = static_cast<int>(screen.a.size());
    if (y < 0 || y >= height || x < 0 || x >= static_cast<int>(screen.a[y].size())) {
        return normal;
    }
    auto ax = x;
    auto ay = y;
    if (term_is_zoom_continuation({ screen.a[y][x], screen.c[y][x].front() })) {
        const auto offset = static_cast<unsigned char>(screen.c[y][x].front()) & 7;
        ax -= offset & 3;
        ay -= offset >> 2;
    }
    if (ax < 0 || ay < 0 || ay + 1 >= height || ax + 3 >= static_cast<int>(screen.a[ay].size()) || ax + 3 >= static_cast<int>(screen.a[ay + 1].size())) {
        return normal;
    }
    if (!(screen.a[ay][ax] & AF_TILE1) || !(screen.c[ay][ax].front() & 0x80) || term_is_tile_continuation({ screen.a[ay][ax], screen.c[ay][ax].front() })) {
        return normal;
    }
    for (auto dy = 0; dy < 2; ++dy) {
        for (auto dx = 0; dx < 4; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            if (screen.a[ay + dy][ax + dx] != 0xf0 || static_cast<unsigned char>(screen.c[ay + dy][ax + dx].front()) != (0x80 | (dy << 2) | dx)) {
                return normal;
            }
        }
    }
    return { ax, ay, 4, 2 };
}

static void mark_tile_block(const TermTileBlock &block)
{
    game_term->y1 = std::min<int>(game_term->y1, block.y);
    game_term->y2 = std::max<int>(game_term->y2, block.y + block.height - 1);
    for (auto row = block.y; row < block.y + block.height; ++row) {
        game_term->x1[row] = std::min<int>(game_term->x1[row], block.x);
        game_term->x2[row] = std::max<int>(game_term->x2[row], block.x + block.width - 1);
    }
}

static bool clear_zoom_tile_at(int x, int y)
{
    // 拡大タイルを描いたことの無い端末には、拡大タイルの範囲のセルは無い
    if (!game_term->has_drawn_zoom_tiles) {
        return false;
    }

    // 拡大タイルの範囲のセルは、左上 (タイル) も続きのセル (0xf0) も属性の最上位ビットが立っている。
    // 立っていないセル (文字のセル) は拡大タイルに含まれないので、範囲を求めずに抜ける
    auto &screen = *game_term->scr;
    if ((y < 0) || (y >= static_cast<int>(screen.a.size())) || (x < 0) || (x >= static_cast<int>(screen.a[y].size())) || !(screen.a[y][x] & AF_TILE1)) {
        return false;
    }

    const auto block = term_tile_block_at(screen, x, y);
    if (!block.zoomed()) {
        return false;
    }
    for (auto dy = 0; dy < 2; ++dy) {
        for (auto dx = 0; dx < 4; ++dx) {
            const auto xx = block.x + dx;
            const auto yy = block.y + dy;
            screen.a[yy][xx] = game_term->attr_blank;
            screen.c[yy][xx] = TermChar::from_byte(game_term->char_blank);
            screen.ta[yy][xx] = 0;
            screen.tc[yy][xx] = 0;
        }
    }
    mark_tile_block(block);
    return true;
}

//! キー入力待ちの間に繰り返し呼ばれるフック (詳細はz-term.hを参照)
void (*term_inkey_wait_hook)() = nullptr;

/*** Local routines ***/

/*!
 * @brief オブジェクトが生存している間、画面表示関数の座標をずらす。
 *
 * 引数でずらすX座標オフセット、Y座標オフセットをそれぞれ指定する。
 * 正方向のオフセットのみ有効。負の値が指定された場合、オフセット位置は 0 とする。
 * 指定された座標が tl::nullopt の場合、現在のオフセットを維持する。
 *
 * @param x X座標オフセット
 * @param y Y座標オフセット
 */
TermOffsetSetter::TermOffsetSetter(tl::optional<TERM_LEN> x, tl::optional<TERM_LEN> y)
    : term(game_term)
    , orig_offset_x(game_term != nullptr ? game_term->offset_x : 0)
    , orig_offset_y(game_term != nullptr ? game_term->offset_y : 0)
{
    if (this->term == nullptr) {
        return;
    }

    if (x) {
        this->term->offset_x = (*x > 0) ? *x : 0;
    }
    if (y) {
        this->term->offset_y = (*y > 0) ? *y : 0;
    }
}

TermOffsetSetter::~TermOffsetSetter()
{
    if (this->term == nullptr) {
        return;
    }

    this->term->offset_x = this->orig_offset_x;
    this->term->offset_y = this->orig_offset_y;
}

/*!
 * @brief オブジェクトが生存している間、画面表示関数の座標をずらす。
 *
 * 表示に使用する領域の大きさを指定し、その領域が画面中央に表示されるように座標をずらす。
 * 引数で領域の横幅、縦幅をそれぞれ指定する。
 * 画面の幅より大きな値が指定された場合はオフセット 0 になる。
 * 指定された幅が tl::nullopt の場合、画面の幅全体を使用する（オフセット 0 になる）。
 *
 * @param width 表示に使用する領域の横幅
 * @param height 表示に使用する領域の縦幅
 */
TermCenteredOffsetSetter::TermCenteredOffsetSetter(tl::optional<TERM_LEN> width, tl::optional<TERM_LEN> height)
    : term(game_term)
    , orig_centered_wid(game_term != nullptr ? game_term->centered_wid : tl::nullopt)
    , orig_centered_hgt(game_term != nullptr ? game_term->centered_hgt : tl::nullopt)
{
    if (game_term == nullptr) {
        return;
    }

    const auto offset_x = width ? (game_term->wid - *width) / 2 : 0;
    const auto offset_y = height ? (game_term->hgt - *height) / 2 : 0;
    this->tos.emplace(offset_x, offset_y);

    game_term->centered_wid = (width < game_term->wid) ? width : tl::nullopt;
    game_term->centered_hgt = (height < game_term->hgt) ? height : tl::nullopt;
}

TermCenteredOffsetSetter::~TermCenteredOffsetSetter()
{
    if (this->term == nullptr) {
        return;
    }

    this->term->centered_wid = this->orig_centered_wid;
    this->term->centered_hgt = this->orig_centered_hgt;
}

/*!
 * Initialize a "term_win" (using the given window size)
 */
term_win::term_win(TERM_LEN w, TERM_LEN h)
    : a(h, std::vector<TERM_COLOR>(w))
    , c(h, std::vector<TermChar>(w))
    , ta(h, std::vector<TERM_COLOR>(w))
    , tc(h, std::vector<char>(w))
{
}

std::unique_ptr<term_win> term_win::create(TERM_LEN w, TERM_LEN h)
{
    //! privateコンストラクタを呼び出すための補助クラス
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
    if (this->a.size() == static_cast<size_t>(h) && this->a[0].size() == static_cast<size_t>(w)) {
        return;
    }

    this->a.resize(h, std::vector<TERM_COLOR>(w));
    this->c.resize(h, std::vector<TermChar>(w));
    this->ta.resize(h, std::vector<TERM_COLOR>(w));
    this->tc.resize(h, std::vector<char>(w));

    for (TERM_LEN y = 0; y < h; y++) {
        this->a[y].resize(w);
        this->c[y].resize(w);
        // 幅を縮めて右のマスが無くなった2マス幅の文字は空白にする
        if ((w > 0) && this->c[y].back().is_wide_lead()) {
            this->c[y].back() = TermChar::from_byte(' ');
        }

        this->ta[y].resize(w);
        this->tc[y].resize(w);
    }

    /* Illegal cursor */
    if (this->cx >= w) {
        this->cu = 1;
    }
    if (this->cy >= h) {
        this->cu = 1;
    }
}

/*!
 * @brief 1マスの色と文字を返す
 * @param x 桁
 * @param y 行
 * @return マスの色と文字。全角文字は左のマスで文字全体を返し、右のマスでは文字を持たない
 */
TermCellSymbol term_win::get_cell(int x, int y) const
{
    return { this->a[y][x], this->c[y][x] };
}

/*** External hooks ***/

void term_user()
{
    if (!game_term->user_hook) {
        return;
    }

    (*game_term->user_hook)(0);
}

void term_xtra(int n, int v)
{
    if (!game_term->xtra_hook) {
        return;
    }

    (*game_term->xtra_hook)(n, v);
}

/*** Fake hooks ***/

/*!
 * Fake hook for "term_curs()" (see above)
 */
static errr term_curs_hack(TERM_LEN x, TERM_LEN y)
{
    /* Unused */
    (void)x;
    (void)y;

    return -1;
}

/*!
 * Fake hook for "term_bigcurs()" (see above)
 */
static errr term_bigcurs_hack(TERM_LEN x, TERM_LEN y)
{
    return (*game_term->curs_hook)(x, y);
}

/*!
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

/*!
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

/*!
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

/*!
 * @brief マスが書き換えられたことを記録する
 * @param x 書き換えたマスの桁
 * @param y 書き換えたマスの行
 */
static void term_mark_changed(TERM_LEN x, TERM_LEN y)
{
    game_term->y1 = std::min(game_term->y1, y);
    game_term->y2 = std::max(game_term->y2, y);
    game_term->x1[y] = std::min(game_term->x1[y], x);
    game_term->x2[y] = std::max(game_term->x2[y], x);
}

/*!
 * @brief マスを書き換える前に、そのマスにかかる2マス幅の文字の、もう片方のマスを空白にする
 * @param x 書き換えるマスの桁
 * @param y 書き換えるマスの行
 * @param keeps_left 左隣のマスを空白にしないなら true (2マス幅の文字の右のマスを書く場合)
 * @details 2マス幅の文字の片方だけが残ると、残ったマスに文字の一部が描かれてしまうため。
 */
static void term_break_wide_char(TERM_LEN x, TERM_LEN y, bool keeps_left)
{
    auto &scr_cc = game_term->scr->c[y];
    if (scr_cc[x].is_wide_trail()) {
        if (!keeps_left && (x > 0)) {
            scr_cc[x - 1] = TermChar::from_byte(' ');
            term_mark_changed(x - 1, y);
        }

        return;
    }

    if (scr_cc[x].is_wide_lead() && (x + 1 < game_term->wid)) {
        scr_cc[x + 1] = TermChar::from_byte(' ');
        term_mark_changed(x + 1, y);
    }
}

/*!
 * @brief 1マスに色と文字を置く
 * @param x マスの桁
 * @param y マスの行
 * @param a 文字の色
 * @param ch 文字
 * @param background 背景のタイル
 * @param preserve_zoom 拡大タイルを消さないなら true
 * @return 画面の内容が変わったならtrue
 */
static bool term_queue_cell(TERM_LEN x, TERM_LEN y, TERM_COLOR a, const TermChar &ch, const DisplaySymbol &background, bool preserve_zoom = false)
{
    if ((x < 0) || (x >= game_term->wid)) {
        return false;
    }
    if ((y < 0) || (y >= game_term->hgt)) {
        return false;
    }
    const auto cleared_zoom = !preserve_zoom && clear_zoom_tile_at(x, y);

    auto &scrn = *game_term->scr;
    auto should_ignore = scrn.a[y][x] == a;
    should_ignore &= scrn.c[y][x] == ch;
    should_ignore &= scrn.ta[y][x] == background.color;
    should_ignore &= scrn.tc[y][x] == background.character;
    if (should_ignore) {
        return cleared_zoom;
    }

    term_break_wide_char(x, y, ch.is_wide_trail());

    scrn.a[y][x] = a;
    scrn.c[y][x] = ch;

    scrn.ta[y][x] = background.color;
    scrn.tc[y][x] = background.character;

    term_mark_changed(x, y);

    // 行頭には左隣が無いので、変更範囲を広げない (x1 が負になり、後の描画で範囲外を読むのを防ぐ)
    if (term_is_zoom_continuation({ a, ch.front() })) {
        const auto offset = static_cast<unsigned char>(ch.front()) & 7;
        const auto ax = x - (offset & 3);
        const auto ay = y - (offset >> 2);
        if (ax >= 0 && ay >= 0 && ax + 3 < game_term->wid && ay + 1 < game_term->hgt) {
            mark_tile_block({ ax, ay, 4, 2 });
        }
    } else if ((x > 0) && (((a & AF_BIGTILE2) == AF_BIGTILE2) || ch.is_wide_trail())) {
        // 大きなタイルや2マス幅の文字の右のマスは、左のマスと一緒に描き直す
        term_mark_changed(x - 1, y);
    }

    return true;
}

/*!
 * Mentally draw an attr/char at a given location
 * Assumes given location and values are valid.
 * @return 画面の内容が変わったならtrue
 */
static bool term_queue_char_aux(TERM_LEN x, TERM_LEN y, const DisplaySymbolPair &symbol_pair, bool preserve_zoom = false)
{
    const auto &symbol_foreground = symbol_pair.symbol_foreground;
    return term_queue_cell(x, y, symbol_foreground.color, TermChar::from_byte(symbol_foreground.character), symbol_pair.symbol_background, preserve_zoom);
}

/*!
 * @brief 描画するマスの範囲の文字を並べた文字列を作る
 * @param win 画面
 * @param x 範囲の先頭の桁
 * @param y 行
 * @param n 範囲のマス数
 * @return text_hook に渡す文字列 (次に呼ぶまで有効)
 * @details 描画のたびに確保しないよう、バッファを使い回す。文字の並べ方は term_win::append_text() を参照。
 */
static const std::string &make_cells_text(const term_win &win, TERM_LEN x, TERM_LEN y, int n)
{
    static std::string text;
    text.clear();
    win.append_text(x, y, n, text);
    return text;
}

/*!
 * @brief pict_hook に渡す、マスごとの文字の先頭バイトを並べた配列を作る
 * @param cc 1行分のマスの文字
 * @param x 範囲の先頭の桁
 * @param n 範囲のマス数
 * @return マスごとの文字の先頭バイト (2マス幅の文字の右のマスは '\0')。次に呼ぶまで有効
 * @details 描画のたびに確保しないよう、バッファを使い回す。
 */
static const std::string &make_cells_front_bytes(const std::vector<TermChar> &cc, TERM_LEN x, int n)
{
    static std::string bytes;
    bytes.clear();
    for (auto i = x; i < x + n; i++) {
        bytes.push_back(cc[i].front());
    }

    return bytes;
}

bool term_queue_scaled_bigchar(int x, int y, const DisplaySymbolPair &symbols, int scale)
{
    if (scale != 2 || !use_bigtile) {
        return term_queue_bigchar(x, y, symbols);
    }
    const auto xx = x + game_term->offset_x;
    const auto yy = y + game_term->offset_y;
    if (xx < 0 || yy < 0 || xx + 3 >= game_term->wid || yy + 1 >= game_term->hgt) {
        return false;
    }
    const auto graphic = (symbols.symbol_foreground.color & 0x80) && (symbols.symbol_foreground.character & 0x80);
    if (graphic) {
        game_term->has_drawn_zoom_tiles = true;
    }

    auto changed = false;
    for (auto dy = 0; dy < 2; ++dy) {
        for (auto dx = 0; dx < 4; ++dx) {
            const auto value = !graphic ? DisplaySymbolPair{ { 0, ' ' }, {} } : dx == 0 && dy == 0 ? symbols
                                                                                                   : DisplaySymbolPair{ { 0xf0, static_cast<char>(0x80 | (dy << 2) | dx) }, {} };
            changed |= term_queue_char_aux(xx + dx, yy + dy, value, graphic);
        }
    }
    return changed;
}

void term_queue_char(TERM_LEN x, TERM_LEN y, const DisplaySymbolPair &symbol_pair)
{
    term_queue_char_aux(x + game_term->offset_x, y + game_term->offset_y, symbol_pair);
}

#ifdef JP
/*!
 * @brief ASCII の文字に対応する全角文字を返す
 * @param ch ASCII の印字可能な文字 (' ' から 0x7f まで)
 * @return 全角文字 (2マス幅の文字の左のマス)
 * @details 0x7f (DEL) には「■」を対応させ、二倍幅豆腐の内部コードに使う。
 */
static const TermChar &ascii_to_zenkaku(char ch)
{
    static const auto zenkaku_chars = [] {
        std::vector<TermChar> chars;
        std::string_view str = "　！”＃＄％＆’（）＊＋，－．／"
                               "０１２３４５６７８９：；＜＝＞？"
                               "＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯ"
                               "ＰＱＲＳＴＵＶＷＸＹＺ［＼］＾＿"
                               "‘ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏ"
                               "ｐｑｒｓｔｕｖｗｘｙｚ｛｜｝～■";
        while (!str.empty()) {
            const auto zenkaku_char = mb_first_char(str);
            chars.push_back(TermChar::wide(zenkaku_char));
            str.remove_prefix(zenkaku_char.length());
        }

        return chars;
    }();

    return zenkaku_chars[ch - ' '];
}
#endif

/*!
 * Bigtile version of term_queue_char().
 * If use_bigtile is FALSE, simply call term_queue_char().
 * Otherwise, mentally draw a pair of attr/char at a given location.
 * Assumes given location and values are valid.
 * @return 画面の内容が変わったならtrue
 */
bool term_queue_bigchar(TERM_LEN x, TERM_LEN y, const DisplaySymbolPair &symbol_pair_initial)
{
    const auto ch_x = x + game_term->offset_x;
    const auto ch_y = y + game_term->offset_y;

    /* If non bigtile mode, call orginal function */
    if (!use_bigtile) {
        return term_queue_char_aux(ch_x, ch_y, symbol_pair_initial);
    }

    /* A tile becomes a Bigtile */
    const auto &symbol_foreground = symbol_pair_initial.symbol_foreground;
    if ((symbol_foreground.color & AF_TILE1) && (symbol_foreground.character & 0x80)) {
        auto symbol_pair = symbol_pair_initial;

        /* Ignore non-tile background */
        if (!((symbol_pair_initial.symbol_background.color & AF_TILE1) && (symbol_pair_initial.symbol_background.character & 0x80))) {
            symbol_pair.symbol_background = {};
        }

        // 右のマスには大きなタイルの印を置く
        const auto is_changed_left = term_queue_char_aux(ch_x, ch_y, symbol_pair);
        const auto is_changed_right = term_queue_char_aux(ch_x + 1, ch_y, { { AF_BIGTILE2, -1 }, {} });
        return is_changed_left || is_changed_right;
    }

#ifdef JP
    /*
     * Use a multibyte character instead of a dirty pair of ASCII
     * characters.
     */
    // 右のマスが画面の外なら、全角文字を置けないので1バイトの文字のまま置く
    const auto ch = static_cast<unsigned char>(symbol_foreground.character);
    if ((' ' <= ch) && (ch <= 0x7f) && (ch_x + 1 < game_term->wid)) {
        const auto &wide_char = ascii_to_zenkaku(symbol_foreground.character);
        const auto is_changed_left = term_queue_cell(ch_x, ch_y, symbol_foreground.color, wide_char, symbol_pair_initial.symbol_background);
        const auto is_changed_right = term_queue_cell(ch_x + 1, ch_y, symbol_foreground.color, TermChar::wide_trail(), {});
        return is_changed_left || is_changed_right;
    }
#endif

    // 1バイトの文字と空白を置く
    const auto is_changed_left = term_queue_char_aux(ch_x, ch_y, symbol_pair_initial);
    const auto is_changed_right = term_queue_char_aux(ch_x + 1, ch_y, { { TERM_WHITE, ' ' }, {} });
    return is_changed_left || is_changed_right;
}

/*!
 * Mentally draw some attr/chars at a given location
 *
 * Assumes that (x,y) is a valid location, that the first "n" characters
 * of the string "s" are all valid (non-zero), and that (x+n-1,y) is also
 * a valid location, so the first "n" characters of "s" can all be added
 * starting at (x,y) without causing any illegal operations.
 */
static void term_queue_chars(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, std::string_view sv)
{
    // 全角文字の片側のマスを上書きしたときに、もう片側を空白にするのは term_queue_cell() が行う
    // タイルの番号は最上位ビットが立っていても1マスに1バイトを置く (属性の最上位ビットで見分ける)
    const auto is_tile = (a & AF_TILE1) != 0;
    while ((n > 0) && !sv.empty()) {
        const auto ch = is_tile ? sv.substr(0, 1) : mb_first_char(sv);
        sv.remove_prefix(ch.length());

        if (!is_tile && is_mb_char_lead(ch.front())) {
            // 後半を描く余地が無いか (n や画面の幅が足りない)、文字が途中で切れている (view の終端か NUL) なら、前半だけでは描かずに終える
            const auto is_truncated = !is_multibyte_char_at(ch) || (ch.find('\0') != std::string_view::npos);
            if ((n < 2) || (x + 1 >= game_term->wid) || is_truncated) {
                // 既存の全角文字の右のマスから書き始めて何も置けなかったときも、その全角文字は空白にする
                if (game_term->scr->c[y][x].is_wide_trail()) {
                    term_queue_cell(x, y, game_term->scr->a[y][x], TermChar::from_byte(' '), {});
                }

                break;
            }

            term_queue_cell(x, y, a, TermChar::wide(ch), {});
            term_queue_cell(x + 1, y, a, TermChar::wide_trail(), {});
            x += 2;
            n -= 2;
            continue;
        }

        term_queue_cell(x, y, a, TermChar::from_byte(ch.front()), {});
        x++;
        n--;
    }
}

/*** Refresh routines ***/

/*!
 * @brief 描き直す範囲の先頭が2マス幅の文字の右のマスなら、左のマスから描き直すよう範囲を広げる
 * @param y 行
 * @param x1 描き直す範囲の先頭の桁
 * @return 広げた範囲の先頭の桁
 */
static TERM_LEN expand_fresh_start(TERM_LEN y, TERM_LEN x1)
{
    if ((x1 > 0) && (game_term->old->c[y][x1].is_wide_trail() || game_term->scr->c[y][x1].is_wide_trail())) {
        return x1 - 1;
    }

    return x1;
}

/*!
 * @brief 描き直すマスの文字の幅を返す
 * @param cc 1行分のマスの文字
 * @param x 桁
 * @return 2マス幅の文字の左のマスなら2 (右のマスと一緒に扱う)、それ以外は1
 */
static int fresh_cell_width(const std::vector<TermChar> &cc, TERM_LEN x)
{
    return (cc[x].is_wide_lead() && (x + 1 < game_term->wid)) ? 2 : 1;
}

/*!
 * @brief 描いたマスと描くマスが同じかを返す
 * @param y 行
 * @param x 先頭の桁
 * @param width マス数
 * @param compares_tiles 背景のタイルも比べるなら true
 * @return 範囲のマスがすべて同じなら true
 */
static bool is_fresh_cells_unchanged(TERM_LEN y, TERM_LEN x, int width, bool compares_tiles)
{
    const auto &old = *game_term->old;
    const auto &scr = *game_term->scr;
    for (auto i = x; i < x + width; i++) {
        if ((scr.a[y][i] != old.a[y][i]) || (scr.c[y][i] != old.c[y][i])) {
            return false;
        }

        if (compares_tiles && ((scr.ta[y][i] != old.ta[y][i]) || (scr.tc[y][i] != old.tc[y][i]))) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief 描くマスを、描いたマスとして覚える
 * @param y 行
 * @param x 先頭の桁
 * @param width マス数
 * @param saves_tiles 背景のタイルも覚えるなら true
 */
static void save_fresh_cells(TERM_LEN y, TERM_LEN x, int width, bool saves_tiles)
{
    auto &old = *game_term->old;
    const auto &scr = *game_term->scr;
    for (auto i = x; i < x + width; i++) {
        old.a[y][i] = scr.a[y][i];
        old.c[y][i] = scr.c[y][i];
        if (saves_tiles) {
            old.ta[y][i] = scr.ta[y][i];
            old.tc[y][i] = scr.tc[y][i];
        }
    }
}

/*!
 * Flush a row of the current window (see "term_fresh")
 * Display text using "term_pict()"
 */
static void term_fresh_row_pict(TERM_LEN y, TERM_LEN x1, TERM_LEN x2)
{
    const auto &scr_aa = game_term->scr->a[y];
    const auto &scr_cc = game_term->scr->c[y];
    const auto &scr_taa = game_term->scr->ta[y];
    const auto &scr_tcc = game_term->scr->tc[y];

    // まとめて描くマスの数と先頭の桁
    TERM_LEN fn = 0;
    TERM_LEN fx = 0;

    const auto flush = [&] {
        if (fn == 0) {
            return;
        }

        const auto &chars = make_cells_front_bytes(scr_cc, fx, fn);
        (void)((*game_term->pict_hook)(fx, y, fn, &scr_aa[fx], chars.data(), &scr_taa[fx], &scr_tcc[fx]));
        fn = 0;
    };

    for (TERM_LEN x = expand_fresh_start(y, x1); x <= x2; x++) {
        // 2マス幅の文字は左右のマスをまとめて扱う
        const auto width = fresh_cell_width(scr_cc, x);
        if (is_fresh_cells_unchanged(y, x, width, true)) {
            flush();
            x += width - 1;
            continue;
        }

        save_fresh_cells(y, x, width, true);
        if (fn == 0) {
            fx = x;
        }

        fn += width;
        x += width - 1;
    }

    flush();
}

/*!
 * @brief 描き直す文字を text_hook で描くか、wipe_hook で消す
 * @param x 先頭の桁
 * @param y 行
 * @param n マス数
 * @param a 色
 * @param win 画面
 */
static void term_fresh_text(TERM_LEN x, TERM_LEN y, int n, TERM_COLOR a, const term_win &win)
{
    if (n == 0) {
        return;
    }

    // 色が黒 (0) なら、always_text でない限り文字を描かずに消す
    if (a || game_term->always_text) {
        const auto &text = make_cells_text(win, x, y, n);
        (void)((*game_term->text_hook)(x, y, n, a, text.data()));
        return;
    }

    (void)((*game_term->wipe_hook)(x, y, n));
}

/*!
 * Flush a row of the current window (see "term_fresh")
 *
 * Display text using "term_text()" and "term_wipe()",
 * but use "term_pict()" for high-bit attr/char pairs
 */
static void term_fresh_row_both(TERM_LEN y, int x1, int x2)
{
    const auto &scr_aa = game_term->scr->a[y];
    const auto &scr_cc = game_term->scr->c[y];
    const auto &scr_taa = game_term->scr->ta[y];
    const auto &scr_tcc = game_term->scr->tc[y];

    // まとめて描くマスの数・先頭の桁・色
    int fn = 0;
    int fx = 0;
    byte fa = game_term->attr_blank;

    const auto flush = [&] {
        term_fresh_text(fx, y, fn, fa, *game_term->scr);
        fn = 0;
    };

    for (TERM_LEN x = expand_fresh_start(y, x1); x <= x2; x++) {
        // 2マス幅の文字は左右のマスをまとめて扱う
        const auto width = fresh_cell_width(scr_cc, x);
        if (is_fresh_cells_unchanged(y, x, width, true)) {
            flush();
            x += width - 1;
            continue;
        }

        save_fresh_cells(y, x, width, true);

        auto na = scr_aa[x];
        if ((na & AF_BIGTILE2) == AF_BIGTILE2) {
            // 大きなタイルの右のマス (拡大タイルの下段には起点がない) は描かないので、手前でまとめて描く
            flush();
            continue;
        }

        // タイル (属性と文字の最上位ビットが立っている) は pict_hook で描く
        if (is_tile_cell(na, scr_cc[x])) {
            flush();
            auto nc = scr_cc[x].front();
            auto nta = scr_taa[x];
            auto ntc = scr_tcc[x];
            (void)((*game_term->pict_hook)(x, y, 1, &na, &nc, &nta, &ntc));
            continue;
        }

        // 色が変わったら、それまでのマスを描く。日本語版では色の上位4ビットを使わない
        if (const byte color = _(na & 0x0f, na); fa != color) {
            flush();
            fa = color;
        }

        if (fn == 0) {
            fx = x;
        }

        fn += width;
        x += width - 1;
    }

    flush();
}

/*!
 * Flush a row of the current window (see "term_fresh")
 *
 * Display text using "term_text()" and "term_wipe()"
 */
static void term_fresh_row_text(TERM_LEN y, TERM_LEN x1, TERM_LEN x2)
{
    const auto &scr_aa = game_term->scr->a[y];
    const auto &scr_cc = game_term->scr->c[y];

    // まとめて描くマスの数・先頭の桁・色
    int fn = 0;
    int fx = 0;
    byte fa = game_term->attr_blank;

    const auto flush = [&] {
        term_fresh_text(fx, y, fn, fa, *game_term->scr);
        fn = 0;
    };

    for (TERM_LEN x = expand_fresh_start(y, x1); x <= x2; x++) {
        // 2マス幅の文字は左右のマスをまとめて扱う。背景のタイルは使わないので比べない
        const auto width = fresh_cell_width(scr_cc, x);
        if (is_fresh_cells_unchanged(y, x, width, false)) {
            flush();
            x += width - 1;
            continue;
        }

        save_fresh_cells(y, x, width, false);

        // 色が変わったら、それまでのマスを描く。日本語版では色の上位4ビットを使わない
        if (const byte color = _(scr_aa[x] & 0x0f, scr_aa[x]); fa != color) {
            flush();
            fa = color;
        }

        if (fn == 0) {
            fx = x;
        }

        fn += width;
        x += width - 1;
    }

    flush();
}

/*!
 * @brief Actually perform all requested changes to the window
 */
void term_fresh()
{
    const auto &old = game_term->old;
    const auto &scr = game_term->scr;
    const auto w = game_term->wid;
    const auto h = game_term->hgt;
    auto y1 = game_term->y1;
    auto y2 = game_term->y2;

    /* Before initialize (Advice from Mr.shimitei)*/
    if (!old || !scr) {
        return;
    }

    if (game_term->never_fresh) {
        return;
    }

    /* Do nothing unless "mapped" */
    if (!game_term->mapped_flag) {
        return;
    }

    /* Trivial Refresh */
    if ((y1 > y2) && (scr->cu == old->cu) && (scr->cv == old->cv) && (scr->cx == old->cx) && (scr->cy == old->cy) && !(game_term->total_erase)) {
        /* Nothing */
        return;
    }

    /* Handle "total erase" */
    if (game_term->total_erase) {
        const auto na = game_term->attr_blank;
        const auto nc = game_term->char_blank;

        /* Physically erase the entire window */
        term_xtra(TERM_XTRA_CLEAR, 0);

        /* clear all "cursor" data */
        old->cv = old->cu = false;
        old->cx = old->cy = 0;

        /* Wipe each row */
        for (auto y = 0; y < h; y++) {
            auto &aa = old->a[y];
            auto &cc = old->c[y];

            auto &taa = old->ta[y];
            auto &tcc = old->tc[y];

            /* Wipe each column */
            for (auto x = 0; x < w; x++) {
                /* Wipe each grid */
                aa[x] = na;
                cc[x] = TermChar::from_byte(nc);

                taa[x] = na;
                tcc[x] = nc;
            }
        }

        /* Redraw every row */
        game_term->y1 = y1 = 0;
        game_term->y2 = y2 = h - 1;

        /* Redraw every column */
        for (auto y = 0; y < h; y++) {
            game_term->x1[y] = 0;
            game_term->x2[y] = w - 1;
        }

        /* Forget "total erase" */
        game_term->total_erase = false;
    }

    /* Cursor update -- Erase old Cursor */
    if (game_term->soft_cursor) {
        /* Cursor was visible */
        if (!old->cu && old->cv) {
            auto tx = old->cx;
            auto ty = old->cy;
            const auto old_block = term_tile_block_at(*old, tx, ty);
            if (old_block.zoomed()) {
                tx = old_block.x;
                ty = old_block.y;
            }

            // カーソルが全角文字の右のマスにあったなら、左のマスから文字全体を描き直す
            if ((tx > 0) && old->c[ty][tx].is_wide_trail()) {
                tx--;
            }

            const auto &old_aa = old->a[ty];
            const auto &old_cc = old->c[ty];

            const auto &old_taa = old->ta[ty];
            const auto &old_tcc = old->tc[ty];
            pict_source = old.get();
            const auto csize = fresh_cell_width(old_cc, tx);

            /* Use "term_pict()" always */
            if (game_term->always_pict) {
                const auto &chars = make_cells_front_bytes(old_cc, tx, csize);
                (void)((*game_term->pict_hook)(tx, ty, csize, &old_aa[tx], chars.data(), &old_taa[tx], &old_tcc[tx]));
            }

            /* Use "term_pict()" sometimes */
            else if (game_term->higher_pict && is_tile_cell(old_aa[tx], old_cc[tx])) {
                auto pict_char = old_cc[tx].front();
                (void)((*game_term->pict_hook)(tx, ty, 1, &old_aa[tx], &pict_char, &old_taa[tx], &old_tcc[tx]));
            }

            /*
             * Restore the actual character
             * 元の文字の描画範囲がカーソルより小さいと、
             * 上書きされなかった部分がゴミとして残る。
             * wipe_hook でカーソルを消去して text_hook で書き直す。
             */
            else if (old_aa[tx] || game_term->always_text) {
                (void)((*game_term->wipe_hook)(tx, ty, csize));
                const auto &text = make_cells_text(*old, tx, ty, csize);
                (void)((*game_term->text_hook)(tx, ty, csize, (unsigned char)(old_aa[tx] & 0xf), text.data()));
            }

            /* Erase the grid */
            else {
                (void)((*game_term->wipe_hook)(tx, ty, csize));
            }
        }
    }

    /* Hide the hardware cursor while drawing */
    else {
        /* Cursor will be invisible */
        term_xtra(TERM_XTRA_SHAPE, 0);
    }

    pict_source = nullptr;

    /* Something to update */
    if (y1 <= y2) {
        /* Handle "icky corner" */
        if (game_term->icky_corner) {
            /* Avoid the corner */
            if (y2 >= h - 1) {
                /* Avoid the corner */
                if (game_term->x2[h - 1] > w - 2) {
                    /* Avoid the corner */
                    game_term->x2[h - 1] = w - 2;
                }

                // 隅の左隣に2マス幅の文字の左のマスがあると、右のマス (隅) と一緒に描いてしまうので、その文字も描かない
                if ((w >= 3) && (game_term->x2[h - 1] == w - 2) && scr->c[h - 1][w - 2].is_wide_lead()) {
                    game_term->x2[h - 1] = w - 3;
                }
            }
        }

        /* Scan the "modified" rows */
        for (TERM_LEN y = y1; y <= y2; ++y) {
            TERM_LEN x1 = game_term->x1[y];
            TERM_LEN x2 = game_term->x2[y];

            /* Flush each "modified" row */
            if (x1 <= x2) {
                /* Always use "term_pict()" */
                if (game_term->always_pict) {
                    /* Flush the row */
                    term_fresh_row_pict(y, x1, x2);
                }

                /* Sometimes use "term_pict()" */
                else if (game_term->higher_pict) {
                    /* Flush the row */
                    term_fresh_row_both(y, x1, x2);
                }

                /* Never use "term_pict()" */
                else {
                    /* Flush the row */
                    term_fresh_row_text(y, x1, x2);
                }

                /* This row is all done */
                game_term->x1[y] = w;
                game_term->x2[y] = 0;

                /* Flush that row (if allowed) */
                if (!game_term->never_frosh) {
                    term_xtra(TERM_XTRA_FROSH, y);
                }
            }
        }

        /* No rows are invalid */
        game_term->y1 = h;
        game_term->y2 = 0;
    }

    /* Cursor update -- Show new Cursor */
    if (game_term->soft_cursor) {
        /* Draw the cursor */
        if (!scr->cu && scr->cv) {
            if ((scr->cx + 1 < w) && ((old->a[scr->cy][scr->cx + 1] & AF_BIGTILE2) == AF_BIGTILE2 || old->c[scr->cy][scr->cx].is_wide_lead())) {
                /* Double width cursor for the Bigtile mode */
                (void)((*game_term->bigcurs_hook)(scr->cx, scr->cy));
            } else {
                /* Call the cursor display routine */
                (void)((*game_term->curs_hook)(scr->cx, scr->cy));
            }
        }
    }

    /* Cursor Update -- Show new Cursor */
    else {
        if (scr->cu) {
            /* Paranoia -- Put the cursor NEAR where it belongs */
            (void)((*game_term->curs_hook)(w - 1, scr->cy));
        } else {
            /* Put the cursor where it belongs */
            (void)((*game_term->curs_hook)(scr->cx, scr->cy));
        }
    }

    /* Save the "cursor state" */
    old->cu = scr->cu;
    old->cv = scr->cv;
    old->cx = scr->cx;
    old->cy = scr->cy;

    /* Actually flush the output */
    term_xtra(TERM_XTRA_FRESH, 0);

    if (!game_term->soft_cursor && !scr->cu && scr->cv) {
        /* The cursor is visible, display it correctly */
        term_xtra(TERM_XTRA_SHAPE, 1);
    }
}

/*!
 * @brief never_freshの値を無視して強制的にterm_freshを行う。
 */
void term_fresh_force()
{
    const auto old = game_term->never_fresh;
    game_term->never_fresh = false;
    term_fresh();
    game_term->never_fresh = old;
}

/*** Output routines ***/

/*!
 * Set the cursor visibility
 */
void term_set_cursor(bool v)
{
    game_term->scr->cv = v;
}

/*!
 * Place the cursor at a given location
 *
 * Note -- "illegal" requests do not move the cursor.
 */
errr term_gotoxy(TERM_LEN x, TERM_LEN y)
{
    int w = game_term->wid;
    int h = game_term->hgt;

    x += game_term->offset_x;
    y += game_term->offset_y;

    /* Verify */
    if ((x < 0) || (x >= w)) {
        return -1;
    }
    if ((y < 0) || (y >= h)) {
        return -1;
    }

    /* Remember the cursor */
    game_term->scr->cx = x;
    game_term->scr->cy = y;

    /* The cursor is not useless */
    game_term->scr->cu = 0;
    return 0;
}

/*!
 * At a given location, place an attr/char
 * Do not change the cursor position
 * No visual changes until "term_fresh()".
 */
void term_draw(int x, int y, const DisplaySymbol &symbol)
{
    if (auto res = term_gotoxy(x, y); res != 0) {
        return;
    }

    /* Paranoia -- illegal char */
    if (!symbol.has_character()) {
        return;
    }

    /* Queue it for later */
    term_queue_char_aux(game_term->scr->cx, game_term->scr->cy, { symbol, {} });
}

/*!
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
void term_addch(const DisplaySymbol &symbol)
{
    TERM_LEN w = game_term->wid;

    /* Handle "unusable" cursor */
    if (game_term->scr->cu) {
        return;
    }

    /* Paranoia -- no illegal chars */
    if (!symbol.has_character()) {
        return;
    }

    /* Queue the given character for display */
    term_queue_char_aux(game_term->scr->cx, game_term->scr->cy, { symbol, {} });

    /* Advance the cursor */
    game_term->scr->cx++;

    /* Success */
    if (game_term->scr->cx < w) {
        return;
    }

    /* Note "Useless" cursor */
    game_term->scr->cu = 1;
}

/*!
 * Bigtile version of term_addch().
 *
 * If use_bigtile is FALSE, simply call term_addch() .
 *
 * Otherwise, queue a pair of attr/char for display at the current
 * cursor location, and advance the cursor to the right by two.
 */
void term_add_bigch(const DisplaySymbol &symbol)
{
    if (!use_bigtile) {
        term_addch(symbol);
        return;
    }

    /* Handle "unusable" cursor */
    if (game_term->scr->cu) {
        return;
    }

    /* Paranoia -- no illegal chars */
    if (!symbol.has_character()) {
        return;
    }

    /* Queue the given character for display */
    term_queue_bigchar(game_term->scr->cx, game_term->scr->cy, { symbol, {} });

    /* Advance the cursor */
    game_term->scr->cx += 2;

    /* Success */
    if (game_term->scr->cx < game_term->wid) {
        return;
    }

    /* Note "Useless" cursor */
    game_term->scr->cu = 1;
}

/*!
 * @brief カーソルの位置に文字列を置き、カーソルを進める
 * @param n 置く最大の表示幅 (マス数)。負なら画面の右端まで
 * @param a 色
 * @param sv 文字列
 * @return カーソルが使えなければ -1。画面の右端に達して途中までしか置けなかったら置いたマス数
 * (以後カーソルは使えなくなる)。それ以外は 0
 * @details 文字列の表示幅と n の小さいほうのマス数を置き、カーソルをそのマス数だけ進める。
 * 画面の右端で折り返さない。属性がタイルなら、1バイトを1マスとして数える。
 */
errr term_addstr(int n, TERM_COLOR a, std::string_view sv)
{
    TERM_LEN w = game_term->wid;
    errr res = 0;

    /* Handle "unusable" cursor */
    if (game_term->scr->cu) {
        return -1;
    }

    /* Obtain maximal length */
    const auto max_len = (n < 0) ? (w + 1) : n;

    // 置くマス数。タイルの番号は1バイトを1マスに置く
    const auto str_width = (a & AF_TILE1) ? sv.length() : str_display_width(sv);
    auto len = std::min<int>(max_len, str_width);

    /* React to reaching the edge of the screen */
    if (game_term->scr->cx + len >= w) {
        res = len = w - game_term->scr->cx;
    }

    /* Queue the first "n" characters for display */
    term_queue_chars(game_term->scr->cx, game_term->scr->cy, len, a, sv);

    /* Advance the cursor */
    game_term->scr->cx += len;

    /* Notice "Useless" cursor */
    if (res) {
        game_term->scr->cu = 1;
    }

    return res;
}

/*!
 * Move to a location and, using an attr, add a char
 */
void term_putch(int x, int y, const DisplaySymbol &symbol)
{
    /* Move first */
    if (term_gotoxy(x, y) != 0) {
        return;
    }

    /* Then add the char */
    term_addch(symbol);
}

/*!
 * Move to a location and, using an attr, add a string
 */
void term_putstr(int x, int y, int n, TERM_COLOR a, std::string_view sv)
{
    if (term_gotoxy(x, y) != 0) {
        return;
    }

    term_addstr(n, a, sv);
}

/*!
 * Place cursor at (x,y), and clear the next "n" chars
 */
void term_erase(int x, int y, tl::optional<int> n_opt)
{
    const auto w = game_term->wid;
    const auto na = game_term->attr_blank;
    const auto nc = TermChar::from_byte(game_term->char_blank);
    auto x1 = -1;
    auto x2 = -1;

    /* Place cursor */
    if (term_gotoxy(x, y) != 0) {
        return;
    }

    x = game_term->scr->cx;
    y = game_term->scr->cy;

    auto n = n_opt.value_or(w);

    /* Force legal size */
    if (x + n > w) {
        n = w - x;
    }
    for (auto column = x; column < x + n; ++column) {
        clear_zoom_tile_at(column, y);
    }

    /* Fast access */
    auto &scr_aa = game_term->scr->a[y];
    auto &scr_cc = game_term->scr->c[y];

    auto &scr_taa = game_term->scr->ta[y];
    auto &scr_tcc = game_term->scr->tc[y];

    /* 2マス幅の文字や大きなタイルの右半分から消去する場合は左半分も消去する。行頭には左隣が無いので何もしない */
    if (n > 0 && x > 0 && (scr_cc[x].is_wide_trail() || (scr_aa[x] & AF_BIGTILE2) == AF_BIGTILE2)) {
        x--;
        n++;
    }

    /* Scan every column */
    for (int i = 0; i < n; i++, x++) {
        /* Ignore "non-changes" */
        if ((scr_aa[x] == na) && (scr_cc[x] == nc)) {
            continue;
        }

        /*
         * 2マス幅の文字の左半分で表示を終了する場合、
         * 重なった文字の右部分を消去。
         *
         * 2001/04/29 -- Habu
         * 行の右端の場合はこの処理をしないように修正。
         */
        if (scr_cc[x].is_wide_lead() && (i + 1) == n && x != w - 1) {
            n++;
        }

        /* Save the "literal" information */
        scr_aa[x] = na;
        scr_cc[x] = nc;

        scr_taa[x] = 0;
        scr_tcc[x] = 0;

        /* Track minimum changed column */
        if (x1 < 0) {
            x1 = x;
        }

        /* Track maximum changed column */
        x2 = x;
    }

    /* Expand the "change area" as needed */
    if (x1 >= 0) {
        /* Check for new min/max row info */
        if (y < game_term->y1) {
            game_term->y1 = y;
        }
        if (y > game_term->y2) {
            game_term->y2 = y;
        }

        /* Check for new min/max col info in this row */
        if (x1 < game_term->x1[y]) {
            game_term->x1[y] = x1;
        }
        if (x2 > game_term->x2[y]) {
            game_term->x2[y] = x2;
        }
    }
}

/*!
 * Clear the entire window, and move to the top left corner
 *
 * Note the use of the special "total_erase" code
 */
void term_clear()
{
    const auto w = game_term->wid;
    const auto h = game_term->hgt;
    DisplaySymbol ds(game_term->attr_blank, game_term->char_blank);

    /* Cursor usable */
    game_term->scr->cu = 0;

    game_term->scr->cx = 0;
    game_term->scr->cy = 0;

    /* Wipe each row */
    for (auto y = 0; y < h; y++) {
        auto &scr_aa = game_term->scr->a[y];
        auto &scr_cc = game_term->scr->c[y];

        auto &scr_taa = game_term->scr->ta[y];
        auto &scr_tcc = game_term->scr->tc[y];

        /* Wipe each column */
        for (auto x = 0; x < w; x++) {
            scr_aa[x] = ds.color;
            scr_cc[x] = TermChar::from_byte(ds.character);

            scr_taa[x] = 0;
            scr_tcc[x] = 0;
        }

        /* This row has changed */
        game_term->x1[y] = 0;
        game_term->x2[y] = w - 1;
    }

    /* Every row has changed */
    game_term->y1 = 0;
    game_term->y2 = h - 1;

    game_term->total_erase = true;
}

/*!
 * Redraw (and refresh) the whole window.
 */
void term_redraw()
{
    game_term->total_erase = true;
    term_fresh();
}

/*!
 * @brief Redraw part of a window.
 *
 * 今のところX11からしか呼ばれていない.
 */
void term_redraw_section(int x1, int y1, int x2, int y2)
{
    /* Bounds checking */
    if (y2 >= game_term->hgt) {
        y2 = game_term->hgt - 1;
    }

    if (x2 >= game_term->wid) {
        x2 = game_term->wid - 1;
    }

    if (y1 < 0) {
        y1 = 0;
    }

    if (x1 < 0) {
        x1 = 0;
    }
    if (x1 > x2 || y1 > y2) {
        return;
    }
    // 拡大タイルの一部を再描画するときは、起点のセルも対象に含める。
    auto expanded = true;
    while (expanded) {
        const std::array<int, 4> before{ { x1, y1, x2, y2 } };
        for (auto row = y1; row <= y2; ++row) {
            for (auto col = x1; col <= x2; ++col) {
                const auto block = term_tile_block_at(*game_term->scr, col, row);
                if (block.zoomed()) {
                    x1 = std::min(x1, block.x);
                    y1 = std::min(y1, block.y);
                    x2 = std::max(x2, block.x + 3);
                    y2 = std::max(y2, block.y + 1);
                }
            }
        }
        expanded = before != std::array<int, 4>{ { x1, y1, x2, y2 } };
    }

    /* Set y limits */
    game_term->y1 = y1;
    game_term->y2 = y2;

    /* Set the x limits */
    for (auto i = game_term->y1; i <= game_term->y2; i++) {
        auto x1j = x1;
        auto x2j = x2;
        if (x1j > 0) {
            auto right_half = (game_term->scr->a[i][x1j] & AF_BIGTILE2) == AF_BIGTILE2 && !term_is_zoom_continuation({ game_term->scr->a[i][x1j], game_term->scr->c[i][x1j].front() });
            right_half |= game_term->scr->c[i][x1j].is_wide_trail();
            if (right_half) {
                x1j--;
            }
        }

        if (x2j < game_term->wid - 1) {
            auto left_half = (game_term->scr->a[i][x2j + 1] & AF_BIGTILE2) == AF_BIGTILE2 && !term_is_zoom_continuation({ game_term->scr->a[i][x2j + 1], game_term->scr->c[i][x2j + 1].front() });
            left_half |= game_term->scr->c[i][x2j].is_wide_lead();
            if (left_half) {
                x2j++;
            }
        }
        game_term->x1[i] = x1j;
        game_term->x2[i] = x2j;

        /* Clear the section so it is redrawn */
        auto &g_ptr = game_term->old->c[i];
        for (auto j = x1j; j <= x2j; j++) {
            /* Hack - set the old character to "none" */
            g_ptr[j] = TermChar();
        }
    }

    term_fresh();
}

/*** Access routines ***/

/*!
 * Extract the cursor visibility
 */
int term_get_cursor()
{
    return game_term->scr->cv;
}

/*!
 * Extract the current window size
 */
std::pair<int, int> term_get_size()
{
    return { game_term->centered_wid.value_or(game_term->wid), game_term->centered_hgt.value_or(game_term->hgt) };
}

/*!
 * Extract the current cursor location
 */
std::pair<int, int> term_locate()
{
    const auto x = game_term->scr->cx - game_term->offset_x;
    const auto y = game_term->scr->cy - game_term->offset_y;
    return { x, y };
}

/*!
 * @brief 現在の端末の1マスの色と文字を返す
 * @param x 桁 (端末のオフセットを加える前)
 * @param y 行 (端末のオフセットを加える前)
 * @return マスの色と文字 (term_win::get_cell() を参照)。画面の外なら tl::nullopt
 * @details 次の term_fresh() の後に画面に出る内容を返す (まだ画面に出ているとは限らない)。
 */
tl::optional<TermCellSymbol> term_what_cell(int x, int y)
{
    x += game_term->offset_x;
    y += game_term->offset_y;
    if ((x < 0) || (x >= game_term->wid) || (y < 0) || (y >= game_term->hgt)) {
        return tl::nullopt;
    }

    return game_term->scr->get_cell(x, y);
}

/*!
 * @brief 1行のマスの範囲の文字を並べて文字列に加える
 * @param x 範囲の先頭の桁
 * @param y 行
 * @param n 範囲のマス数
 * @param text 加える先の文字列
 * @details 全角文字は左のマスで文字全体を加える。範囲の端で分断された全角文字や、片側のマスしか無い全角文字は空白にする。
 */
void term_win::append_text(int x, int y, int n, std::string &text) const
{
    const auto &cc = this->c[y];
    for (auto i = x; i < x + n; i++) {
        const auto &ch = cc[i];
        if (ch.is_wide_lead()) {
            const auto has_trail = (i + 1 < x + n) && cc[i + 1].is_wide_trail();
            text.append(has_trail ? ch.view() : " ");
            continue;
        }

        if (ch.is_wide_trail()) {
            const auto has_lead = (i > x) && cc[i - 1].is_wide_lead();
            if (!has_lead) {
                text.push_back(' ');
            }

            continue;
        }

        text.append(ch.view());
    }
}

/*** Input routines ***/

/*!
 * Flush and forget the input
 */
void term_flush()
{
    /* Flush all events */
    term_xtra(TERM_XTRA_FLUSH, 0);

    /* Forget all keypresses */
    game_term->key_head = game_term->key_tail = 0;
}

/*!
 * Add a keypress to the FRONT of the "queue"
 */
errr term_key_push(int k)
{
    /* Refuse to enqueue non-keys */
    if (!k) {
        return -1;
    }

    /* Overflow may induce circular queue */
    if (game_term->key_tail == 0) {
        game_term->key_tail = game_term->key_size;
    }

    /* Back up, Store the char */
    game_term->key_queue[--game_term->key_tail] = (char)k;

    if (game_term->key_head != game_term->key_tail) {
        return 0;
    }

    return 1;
}

/*!
 * @brief キューに積まれているキーの数を数える
 * @param term 数える端末
 * @return 積まれているキーの数
 * @details キューは循環しているので、先頭が末尾より前にあることもある。
 */
int term_key_queue_pending(const term_type &term)
{
    const auto size = term.key_size;
    if (size == 0) {
        return 0;
    }

    return (term.key_head + size - term.key_tail) % size;
}

/*!
 * @brief キューにあと何個キーを積めるか数える
 * @return 積めるキーの数
 * @details
 * term_key_push()は空きが無くなっても積むことを拒否せず、循環キューを一周して
 * 先に積んだキーを壊すため、まとめて積む場合は事前に空きを確かめる必要がある。
 * キューの大きさはフロントエンドが決めるため、一度に積める量もそれに従う。
 */
int term_key_queue_room()
{
    const auto size = game_term->key_size;
    if (size == 0) {
        return 0;
    }

    return size - 1 - term_key_queue_pending(*game_term);
}

/*!
 * @brief キー列をキューへまとめて積む
 * @param keys 積むキー列
 * @return 全て積めた場合0、空きが足りず1つも積まなかった場合は非0
 * @details
 * term_key_push()はキューの先頭へ挿入するため、正しい順序で消費させるには
 * 末尾から逆順に積む必要がある。逆順に積む都合上、途中で空きが尽きると
 * 失われるのはキー列の先頭側になる。意味の異なる操作を実行させないよう、
 * 全て積めない場合は1つも積まない。
 */
errr term_keys_push(std::string_view keys)
{
    if (std::cmp_greater(keys.size(), term_key_queue_room())) {
        return 1;
    }

    for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
        (void)term_key_push(static_cast<unsigned char>(*it));
    }

    return 0;
}

/*!
 * @brief 保留中のイベントを1件だけ取り出す
 * @return 取り出した場合0、保留中のイベントが無い場合または取り出せない場合は非0
 * @details
 * term_xtra()は戻り値を捨てるため、イベントを取り切ったか否かを判定できない。
 * ポーリングによる入力待ちでは打ち切る条件が要るため、ここだけフックを直接呼ぶ。
 */
static errr take_pending_event()
{
    if (!game_term->xtra_hook) {
        return 1;
    }

    return (*game_term->xtra_hook)(TERM_XTRA_EVENT, false);
}

/*!
 * @brief Check for a pending keypress on the key queue.
 * @param wait Wait for a keypress.
 * @param take Remove the keypress.
 * @return Keypress or zero.
 */
char term_inkey(bool wait, bool take)
{
    /* get bored */
    if (!game_term->never_bored) {
        /* Process random events */
        term_xtra(TERM_XTRA_BORED, 0);
    }

    if (wait) {
        while (game_term->key_head == game_term->key_tail) {
            if (term_inkey_wait_hook == nullptr) {
                term_xtra(TERM_XTRA_EVENT, true);
                continue;
            }

            // フックに仕事をさせるため、フロントエンドをブロックさせずに
            // 保留中のイベントを取り切ってから制御を渡す
            while ((game_term->key_head == game_term->key_tail) && (take_pending_event() == 0)) {
                ;
            }

            // キーが届いていれば外側のループが抜ける。無駄に待たせないようフックは呼ばない
            if (game_term->key_head == game_term->key_tail) {
                (*term_inkey_wait_hook)();
            }
        }
    } else if (game_term->key_head == game_term->key_tail) {
        term_xtra(TERM_XTRA_EVENT, false);
    }

    /* No keys are ready */
    if (game_term->key_head == game_term->key_tail) {
        return '\0';
    }

    /* If requested, advance the queue, wrap around if necessary */
    const auto res = game_term->key_queue[game_term->key_tail];
    if (take && (++game_term->key_tail == game_term->key_size)) {
        game_term->key_tail = 0;
    }

    return res;
}

/*** Extra routines ***/

/*!
 * Save the "requested" screen into the "memorized" screen
 *
 * Every "term_save()" should match exactly one "term_load()"
 */
void term_save()
{
    game_term->mem_stack.push(game_term->scr->clone());
}

/*!
 * Restore the "requested" contents (see above).
 *
 * Every "term_save()" should match exactly one "term_load()"
 */
void term_load(bool should_load_all)
{
    const auto w = game_term->wid;
    const auto h = game_term->hgt;
    if (game_term->mem_stack.empty()) {
        return;
    }

    if (should_load_all) {
        // 残り1つを残して読み捨てる
        while (game_term->mem_stack.size() > 1) {
            game_term->mem_stack.pop();
        }
    }

    /* Load */
    game_term->scr.swap(game_term->mem_stack.top());
    game_term->scr->resize(w, h);

    /* Pop stack */
    game_term->mem_stack.pop();

    /* Assume change */
    for (auto y = 0; y < h; y++) {
        /* Assume change */
        game_term->x1[y] = 0;
        game_term->x2[y] = w - 1;
    }

    /* Assume change */
    game_term->y1 = 0;
    game_term->y2 = h - 1;
}

/*!
 * React to a new physical window size.
 */
void term_resize(int w, int h)
{
    /* Resizing is forbidden */
    if (game_term->fixed_shape) {
        return;
    }

    /* Ignore illegal changes */
    if ((w < 1) || (h < 1)) {
        return;
    }

    /* Ignore non-changes */
    if ((game_term->wid == w) && (game_term->hgt == h) && (arg_bigtile == use_bigtile)) {
        return;
    }

    use_bigtile = arg_bigtile;

    /* Resize windows */
    game_term->old->resize(w, h);
    game_term->scr->resize(w, h);
    if (game_term->tmp) {
        game_term->tmp->resize(w, h);
    }

    /* Resize scanners */
    game_term->x1.resize(h);
    game_term->x2.resize(h);

    /* Save new size */
    game_term->wid = w;
    game_term->hgt = h;

    /* Force "total erase" */
    game_term->total_erase = true;

    /* Assume change */
    for (auto i = 0; i < h; i++) {
        /* Assume change */
        game_term->x1[i] = 0;
        game_term->x2[i] = w - 1;
    }

    /* Assume change */
    game_term->y1 = 0;
    game_term->y2 = h - 1;

    /* Execute the "resize_hook" hook, if available */
    if (game_term->resize_hook) {
        game_term->resize_hook();
    }
}

/*!
 * Activate a new Term (and deactivate the current Term)
 *
 * This function is extremely important, and also somewhat bizarre.
 * It is the only function that should "modify" the value of "Term".
 *
 * To "create" a valid "term", one should do "term_init(t)", then
 * set the various flags and hooks, and then do "term_activate(t)".
 */
void term_activate(term_type *t)
{
    /* already done */
    if (game_term == t) {
        return;
    }

    /* Deactivate the old Term */
    if (game_term) {
        term_xtra(TERM_XTRA_LEVEL, 0);
    }

    /* Call the special "init" hook */
    if (t && !t->active_flag) {
        /* Call the "init" hook */
        if (t->init_hook) {
            (*t->init_hook)(t);
        }

        /* Remember */
        t->active_flag = true;

        /* Assume mapped */
        t->mapped_flag = true;
    }

    /* Remember the Term */
    game_term = t;

    /* Activate the new Term */
    if (game_term) {
        term_xtra(TERM_XTRA_LEVEL, 1);
    }
}

/*!
 * Initialize a term, using a window of the given size.
 * Also prepare the "input queue" for "k" keypresses
 * By default, the cursor starts out "invisible"
 * By default, we "erase" using "black spaces"
 */
void term_init(term_type *t, int w, int h, int k)
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
}

#ifdef JP
/*!
 * @brief 文字列を縦書きで置く
 * @param x 桁
 * @param y_initial 先頭の文字を置く行
 * @param color 色
 * @param sv 文字列。NUL があればそこで終える
 * @details 1文字ずつ下の行に置く。画面の右端や下端に達したら、そこで終える。
 */
void term_putstr_v(int x, int y_initial, uint8_t color, std::string_view sv)
{
    for (auto y = y_initial; !sv.empty() && (sv.front() != '\0'); y++) {
        if (term_gotoxy(x, y) != 0) {
            return;
        }

        const auto ch = mb_first_char(sv);
        sv.remove_prefix(ch.length());
        if (term_addstr(-1, color, ch) != 0) {
            return;
        }
    }
}
#endif

#ifndef _WIN32
errr term_nuke(term_type *t)
{
    if (t->active_flag) {
        if (t->nuke_hook) {
            (*t->nuke_hook)(t);
        }

        t->active_flag = false;
        t->mapped_flag = false;
    }

    t->old.reset();
    t->scr.reset();
    t->tmp.reset();
    while (!t->mem_stack.empty()) {
        t->mem_stack.pop();
    }

    t->x1.clear();
    t->x2.clear();
    t->key_queue.clear();
    return 0;
}
#endif
