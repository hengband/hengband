/*!
 * @file input-context.h
 * @brief 入力待ちの文脈のヘッダ
 * @details
 * ゲームが待っている入力の種類とプロンプトを、入力待ちの間だけ保持する。
 * 文脈を設定するのは対応した入力待ちだけなので、文脈が無いことは入力を待っていないことを意味しない。
 */

#pragma once

#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <variant>

/*!
 * @brief 待っている入力の種類
 */
enum class InputKind {
    CONFIRM, //!< [y/n] などの確認
    COMMAND_CHAR, //!< 1文字の入力
    STRING, //!< 文字列の入力
    QUANTITY, //!< 数量の入力
    MORE, //!< メッセージの -続く- (-more-)
    COMMAND, //!< 次のコマンドの入力 (自分のターン、店のコマンド)
};

/*!
 * @brief 文字列の入力の詳細
 */
struct StringInputDetails {
    int max_length; //!< 入力欄の位置と画面幅に合わせて調整した、内部文字コードでのバイト上限
    std::string initial; //!< 初期文字列 (max_length に合わせた切り詰めはしない)
};

/*!
 * @brief 数量の入力の詳細
 */
struct QuantityInputDetails {
    int max; //!< 数量の上限
    int initial; //!< 初期数量
};

/*!
 * @brief コマンドの入力の詳細
 */
struct CommandInputDetails {
    bool shopping; //!< 店のコマンドの入力か
};

//! 入力の種類ごとの詳細。詳細を持たない入力では std::monostate
using InputDetails = std::variant<std::monostate, StringInputDetails, QuantityInputDetails, CommandInputDetails>;

/*!
 * @brief 現在待っている入力の種類と、画面に表示したプロンプト
 */
struct InputContext {
    InputKind kind; //!< 入力の種類
    std::string prompt; //!< 画面に表示したプロンプト
    InputDetails details; //!< 入力の種類ごとの詳細
};

/*!
 * @brief 入力待ちの間だけ文脈を公開し、終了時に以前の文脈へ戻す
 */
class InputContextScope {
public:
    InputContextScope(InputKind kind, std::string_view prompt);
    explicit InputContextScope(InputContext input_context);
    ~InputContextScope();

    InputContextScope(const InputContextScope &) = delete;
    InputContextScope &operator=(const InputContextScope &) = delete;

private:
    tl::optional<InputContext> previous;
};

const tl::optional<InputContext> &get_input_context();
