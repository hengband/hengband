/*!
 * @file bot-control-server.h
 * @brief 外部プログラムからゲームを操作する制御サーバのヘッダ
 */

#pragma once

#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>

struct TermSize;

/*!
 * @brief 現在待っている入力の種類と、画面に表示したプロンプト
 */
struct BotInputContext {
    std::string kind;
    std::string prompt;
};

/*!
 * @brief 入力待ちの間だけ文脈を公開し、終了時に以前の文脈へ戻す
 */
class BotInputContextScope {
public:
    BotInputContextScope(std::string_view kind, std::string_view prompt);
    ~BotInputContextScope();

    BotInputContextScope(const BotInputContextScope &) = delete;
    BotInputContextScope &operator=(const BotInputContextScope &) = delete;

private:
    tl::optional<BotInputContext> previous;
};

const tl::optional<BotInputContext> &get_bot_input_context();

/*!
 * @brief 1リクエストで注入できるキーの最大数
 * @details
 * これを1度に積み切れるキーキューを持たない端末では、上限に届く前に
 * 空きが足りずキー列が拒否される。端末側がキューの大きさを決める際の目安。
 */
constexpr auto BOT_CONTROL_MAX_KEYS = 1023;

/*!
 * @brief 端末の大きさを変える関数
 * @details 引数は angband_terms 上の添字と新しい大きさ。失敗した場合は理由を返す。
 */
using BotTermResizer = tl::expected<void, std::string> (*)(int index, const TermSize &size);

void set_bot_term_resizer(BotTermResizer resizer);
void init_bot_control_server();
void shutdown_bot_control_server();
