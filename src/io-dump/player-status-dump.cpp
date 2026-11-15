#include "io-dump/player-status-dump.h"
#include "term/z-term.h"
#include "view/display-player.h"

/*!
 * @brief 画面番号を指定してダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @param display_player 画面表示へのコールバック
 * @param mode 表示モード
 * @param start_y ダンプの開始行数
 * @param tmp_end_y ダンプの終了行数 (補正前)
 * @param change_color バッファへ詰める文字の変更有無
 * @details モンスター名の長さによっては固定長だとはみ出す可能性があるので、適切に終了行数を調整する.
 */
static void dump_player_status_with_screen_num(PlayerType *player_ptr, FILE *fff,
    const int mode, const TERM_LEN start_y, const TERM_LEN tmp_end_y, const bool change_color)
{
    auto num_lines = display_player(player_ptr, mode);
    auto end_y = tmp_end_y + num_lines.value_or(0);
    for (auto y = start_y; y < end_y; y++) {
        constexpr auto line_width = 79;
        std::string line;
        for (TERM_LEN x = 0; x < line_width; x++) {
            const auto cell = term_what_cell(x, y).value_or(TermCellSymbol{ 0, TermChar::from_byte('\0') });

            // 全角文字は左のマスで文字全体を書く。右のマスが行の範囲の外にはみ出す全角文字は空白にする
            const auto is_cut_off = cell.character.is_wide_lead() && (x + 1 >= line_width);
            line.append((is_cut_off || (change_color && (cell.color >= 128))) ? std::string_view(" ") : cell.character.view());
        }

        // 末尾の空白を除いた後、NUL のマスがあればそこで行を終える
        while (line.ends_with(' ')) {
            line.pop_back();
        }

        fmt::println(fff, "{}", line.substr(0, line.find('\0')));
    }
}

/*!
 * @brief プレイヤーのステータス表示をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
void dump_aux_player_status(PlayerType *player_ptr, FILE *fff)
{
    dump_player_status_with_screen_num(player_ptr, fff, 0, 1, 22, false);
    dump_player_status_with_screen_num(player_ptr, fff, 1, 10, 17, false);
    fmt::print(fff, "\n");
    dump_player_status_with_screen_num(player_ptr, fff, 2, 2, 22, true);
    fmt::print(fff, "\n");
    dump_player_status_with_screen_num(player_ptr, fff, 3, 1, 18, true);
    fmt::print(fff, "\n");
    dump_player_status_with_screen_num(player_ptr, fff, 4, 1, 19, true);
    fmt::print(fff, "\n");
}
