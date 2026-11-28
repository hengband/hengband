#include "cmd-io/cmd-autopick.h"
#include "autopick/autopick-command-menu.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-drawer.h"
#include "autopick/autopick-editor-command.h"
#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-initializer.h"
#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-util.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "io/input-context.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "locale/multibyte-char.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/finalizer.h"
#include "util/int-char-converter.h"
#include "world/world.h"
#include <fmt/format.h>
#include <set>

AutopickTextBody::AutopickTextBody(int cx, int cy)
    : cx(cx)
    , cy(cy)
    , lines(1)
{
}

/*
 * Check special key code and get a movement command id
 */
static EditorCommandId analyze_move_key(AutopickTextBody *tb, uint32_t skey)
{
    EditorCommandId com_id;
    if (!(skey & SKEY_MASK)) {
        return EditorCommandId::NONE;
    }

    switch (skey & ~SKEY_MOD_MASK) {
    case SKEY_DOWN:
        com_id = EditorCommandId::DOWN;
        break;
    case SKEY_LEFT:
        com_id = EditorCommandId::LEFT;
        break;
    case SKEY_RIGHT:
        com_id = EditorCommandId::RIGHT;
        break;
    case SKEY_UP:
        com_id = EditorCommandId::UP;
        break;
    case SKEY_PGUP:
        com_id = EditorCommandId::PGUP;
        break;
    case SKEY_PGDOWN:
        com_id = EditorCommandId::PGDOWN;
        break;
    case SKEY_HOME:
        com_id = any_bits(skey, SKEY_MOD_CONTROL) ? EditorCommandId::TOP : EditorCommandId::BOL;
        break;
    case SKEY_END:
        com_id = any_bits(skey, SKEY_MOD_CONTROL) ? EditorCommandId::BOTTOM : EditorCommandId::EOL;
        break;
    default:
        return EditorCommandId::NONE;
    }

    if (!(skey & SKEY_MOD_SHIFT)) {
        /*
         * Un-shifted cursor keys cancells
         * selection created by shift+cursor.
         */
        if (tb->marks.has(AutopickMark::BY_SHIFT)) {
            tb->marks.clear();
            tb->dirty_flags |= DIRTY_ALL;
        }

        return com_id;
    }

    if (tb->marks.any()) {
        return com_id;
    }

    const int len = tb->lines[tb->cy].line.length();
    tb->marks.set(AutopickMark::MARK);
    tb->marks.set(AutopickMark::BY_SHIFT);
    tb->my = tb->cy;
    tb->mx = tb->cx;
    if (tb->cx > len) {
        tb->mx = len;
    }

    if (com_id == EditorCommandId::UP || com_id == EditorCommandId::DOWN) {
        tb->dirty_flags |= DIRTY_ALL;
    } else {
        tb->dirty_line = tb->cy;
    }

    return com_id;
}

namespace {
/*!
 * @brief 行の中で表示桁にあたる位置を返す
 * @param line 行
 * @param column 表示桁
 * @return 表示桁にあたる位置 (バイト)。表示桁が全角文字の右半分にあたるときは、表示桁が奇数ならその文字の先頭、
 * 偶数なら次の文字の先頭 (描画の前にカーソルを文字の境界へ動かすときと同じ)。行より右なら行の長さ
 */
int find_column_position(std::string_view line, int column)
{
    size_t pos = 0;
    auto col = 0;
    while ((pos < line.length()) && (col < column)) {
        const auto ch = mb_first_char(line.substr(pos));
        const auto width = static_cast<int>(str_display_width(ch));
        if (col + width > column) {
            return static_cast<int>((column & 1) ? pos : pos + ch.length());
        }

        pos += ch.length();
        col += width;
    }

    return static_cast<int>(pos);
}
}

/*!
 * @brief コマンドを処理する前にカーソル列とマーク列を行の範囲内に収める
 * @details 上下に移動している間は、覚えておいた表示桁にカーソルを置く。
 * キーワードの切り替え等はマークを残したまま行を短くするため、マーク中はマーク列も補正する。
 * マークを外した後の my / mx は存在しない行を指していることがあるため触らない。
 */
void AutopickTextBody::adjust_cursor_column()
{
    if (this->rec_cx > 0) {
        this->cx = find_column_position(this->lines[this->cy].line, this->rec_cx);
    }

    this->cx = std::min<int>(this->lines[this->cy].line.length(), this->cx);
    if (this->marks.any()) {
        this->mx = std::min<int>(this->lines[this->my].line.length(), this->mx);
    }
}

/*!
 * @brief 上下に移動するコマンドなら、カーソルの表示桁を覚える
 * @param com_id 処理したコマンド
 * @param cx_before コマンドを処理する前のカーソルの位置 (バイト)
 * @param column_before コマンドを処理する前のカーソルの表示桁
 * @details カーソルの位置を変えないコマンド (上下の移動) なら処理する前の行での表示桁を、
 * 変えたコマンド (行頭へ移る BOTTOM) なら移った先の行での表示桁を覚える。
 */
void AutopickTextBody::update_cursor_column_record(EditorCommandId com_id, int cx_before, int column_before)
{
    static const std::set<EditorCommandId> record_cursor_column_commands = { EditorCommandId::UP, EditorCommandId::DOWN, EditorCommandId::PGUP, EditorCommandId::PGDOWN, EditorCommandId::TOP, EditorCommandId::BOTTOM };
    if (!record_cursor_column_commands.contains(com_id)) {
        this->rec_cx = 0;
        return;
    }

    const auto column = (this->cx == cx_before) ? column_before : this->get_cursor_column();
    this->rec_cx = std::max(column, this->rec_cx);
}

/*!
 * @brief カーソルの表示桁を返す
 * @return カーソルより左にある文字の表示幅
 */
int AutopickTextBody::get_cursor_column() const
{
    const std::string_view line(this->lines[this->cy].line);
    return static_cast<int>(str_display_width(line.substr(0, std::min<size_t>(this->cx, line.length()))));
}

/*!
 * @brief カーソルが多バイト文字の途中にあるかどうかを判定する
 * @return カーソルのある行で、カーソルの位置が多バイト文字の後続のバイトであれば true
 */
bool AutopickTextBody::is_cursor_inside_multibyte_char() const
{
    return is_inside_multibyte_char(this->lines[this->cy].line, this->cx);
}

/*!
 * @brief 指定の行があるかどうかを判定する
 * @param y 行番号
 * @return 指定の行があれば true
 */
bool AutopickTextBody::has_line(int y) const
{
    return (0 <= y) && (y < std::ssize(this->lines));
}

/*!
 * @brief 指定の行が最終行かどうかを判定する
 * @param y 行番号
 * @return 指定の行より後ろに行が無ければ true
 */
bool AutopickTextBody::is_last_line(int y) const
{
    return !this->has_line(y + 1);
}

/*!
 * @brief 行をすべて差し替える
 * @param new_lines 新しい行
 * @details 行が無ければ空行を 1 行だけ持つ。各行の状態は初期化するので、描き直しの前に DIRTY_EXPRESSION で求め直すこと。
 */
void AutopickTextBody::set_lines(std::vector<std::string> new_lines)
{
    this->lines.clear();
    this->lines.reserve(new_lines.size());
    for (auto &line : new_lines) {
        this->lines.emplace_back().line = std::move(line);
    }

    if (this->lines.empty()) {
        this->lines.emplace_back();
    }
}

/*
 * In-game editor of Object Auto-picker/Destoryer
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_edit_autopick(PlayerType *player_ptr)
{
    static int cx_save = 0;
    static int cy_save = 0;
    autopick_type an_entry, *entry = &an_entry;
    int key = -1;
    static int32_t old_autosave_turn = 0L;
    ape_quittance quit = APE_QUIT;

    AutopickTextBody text_body(cx_save, cy_save);
    AutopickTextBody *tb = &text_body;
    tb->dirty_flags = DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
    tb->filename_mode = PickType::DEFAULT;
    auto &world = AngbandWorld::get_instance();
    world.play_time.pause();
    const auto unpauser = util::make_finalizer([&world]() { world.play_time.unpause(); });
    if (world.game_turn < old_autosave_turn) {
        while (old_autosave_turn > world.game_turn) {
            old_autosave_turn -= TURNS_PER_DAY;
        }
    }

    if (world.game_turn > old_autosave_turn + 100L) {
        do_cmd_save_game(player_ptr, true);
        old_autosave_turn = world.game_turn;
    }

    init_autopick();
    if (autopick_last_destroyed_object.is_valid()) {
        autopick_entry_from_object(player_ptr, entry, &autopick_last_destroyed_object);
        tb->last_destroyed = autopick_line_from_entry(*entry);
    }

    auto [filename_mode, lines] = read_pickpref_text_lines(player_ptr->base_name);
    tb->filename_mode = filename_mode;
    tb->set_lines(std::move(lines));

    // 前回閉じたときのカーソルの行が、読み込み直したファイルに無ければ先頭に戻す
    if (!tb->has_line(tb->cy)) {
        tb->cy = tb->cx = 0;
    }

    TermCenteredOffsetSetter tcos(tl::nullopt, tl::nullopt);

    screen_save();
    while (quit == APE_QUIT) {
        auto com_id = EditorCommandId::NONE;
        tb->adjust_cursor_column();
        draw_text_editor(player_ptr, tb);
        constexpr auto key_guide = _("(^Q:終了 ^W:セーブして終了, ESC:メニュー, その他:入力)", "(^Q:Quit, ^W:Save&Quit, ESC:Menu, Other:Input text)");
        prt(key_guide, 0, 0);
        if (tb->marks.none()) {
            prt(fmt::format("({},{})", tb->cx, tb->cy), 0, 60);
        } else {
            prt(fmt::format("({},{})-({},{})", tb->mx, tb->my, tb->cx, tb->cy), 0, 60);
        }

        const auto cx_before = tb->cx;
        const auto cursor_column = tb->get_cursor_column();
        term_gotoxy(cursor_column - tb->left, tb->cy - tb->upper + 1);
        tb->dirty_flags = 0;
        tb->dirty_line = -1;
        tb->old_cy = tb->cy;
        tb->old_upper = tb->upper;
        tb->old_left = tb->left;
        tb->old_wid = tb->wid;
        tb->old_hgt = tb->hgt;

        key = input_special_key_with_context(InputKind::AUTOPICK_EDITOR, key_guide);

        if (key & SKEY_MASK) {
            com_id = analyze_move_key(tb, static_cast<uint32_t>(key));
        } else if (key == ESCAPE) {
            com_id = do_command_menu(0, 0);
            tb->dirty_flags |= DIRTY_SCREEN;
        } else if (!iscntrl((unsigned char)key)) {
            insert_single_letter(tb, key);
        } else {
            com_id = CommandMenuData::get_instance().get_com_id((char)key);
        }

        if (com_id > EditorCommandId::NONE) {
            quit = do_editor_command(player_ptr, tb, com_id);
        }

        tb->update_cursor_column_record(com_id, cx_before, cursor_column);
    }

    screen_load();
    const auto filename = pickpref_filename(player_ptr->base_name, tb->filename_mode);

    if (quit == APE_QUIT_AND_SAVE) {
        write_text_lines(filename, tb->get_entry_lines());
    }

    process_autopick_file(player_ptr, filename);
    cx_save = tb->cx;
    cy_save = tb->cy;
}
