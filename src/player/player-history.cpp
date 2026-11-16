#include "player/player-history.h"
#include "util/buffer-shaper.h"
#include <algorithm>
#include <iterator>

/*!
 * @brief 文字列を生い立ちの行に区切る
 * @param text 生い立ちの文章
 * @return 表示幅 PLAYER_HISTORY_WIDTH 桁ごとに区切った生い立ち。PLAYER_HISTORY_LINES 行を超えた分は捨てる
 */
PlayerHistory shape_player_history(std::string_view text)
{
    PlayerHistory history{};
    auto lines = shape_buffer(text, PLAYER_HISTORY_WIDTH + 1); // 表示幅 PLAYER_HISTORY_WIDTH 桁ごとに区切る
    std::copy_n(std::make_move_iterator(lines.begin()), std::min(history.size(), lines.size()), history.begin());
    return history;
}
