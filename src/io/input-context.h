/*!
 * @file input-context.h
 * @brief 入力待ちの文脈のヘッダ
 * @details
 * ゲームが待っている入力の種類とプロンプトを、入力待ちの間だけ保持する。
 * 文脈を設定するのは対応した入力待ちだけなので、文脈が無いことは入力を待っていないことを意味しない。
 */

#pragma once

#include "util/point-2d.h"
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
    ITEM, //!< アイテムの選択
    DIRECTION, //!< 方向の入力
    TARGET, //!< ターゲットの選択と、見るコマンドのカーソル
};

/*!
 * @brief 文字列の入力の詳細
 */
struct StringInputDetails {
    int max_length; //!< 入力欄の位置と画面幅に合わせて調整した、内部文字コードでのバイト上限
    std::string initial; //!< 初期文字列 (max_length に合わせた切り詰めはしない)

    bool operator==(const StringInputDetails &) const = default;
};

/*!
 * @brief 数量の入力の詳細
 */
struct QuantityInputDetails {
    int max; //!< 数量の上限
    int initial; //!< 初期数量

    bool operator==(const QuantityInputDetails &) const = default;
};

/*!
 * @brief コマンドの入力の詳細
 */
struct CommandInputDetails {
    bool shopping; //!< 店のコマンドの入力か

    bool operator==(const CommandInputDetails &) const = default;
};

/*!
 * @brief アイテムを選ぶ区分
 */
enum class ItemSelectionArea {
    INVENTORY, //!< 持ち物
    EQUIPMENT, //!< 装備品
    FLOOR, //!< 足元
};

/*!
 * @brief アイテムの選択の詳細
 */
struct ItemInputDetails {
    ItemSelectionArea area; //!< いま選んでいる区分
    bool inventory; //!< 持ち物から選べるか
    bool equipment; //!< 装備品から選べるか
    bool floor; //!< 足元から選べるか
    char first_label; //!< いま選んでいる区分のラベルの範囲の先頭 (範囲の中にも選べないアイテムがありうる。銘のタグと練気術の w はラベルより優先する)
    char last_label; //!< いま選んでいる区分のラベルの範囲の末尾
    bool force; //!< w で練気術を選べるか
    bool menu; //!< コマンドメニューから選んだコマンドで、ラベルではなくカーソルで選ぶか

    bool operator==(const ItemInputDetails &) const = default;
};

/*!
 * @brief 方向の入力の詳細
 */
struct DirectionInputDetails {
    bool can_choose_target; //!< '*' などでターゲットを選び直せるか
    bool has_target; //!< '5' などで今のターゲットへ向けられるか
    bool at_feet; //!< '.' などで足元を選べるか

    bool operator==(const DirectionInputDetails &) const = default;
};

/*!
 * @brief ターゲットの選択の詳細
 */
struct TargetInputDetails {
    bool look; //!< 見るコマンドか (false ならターゲットの選択)
    bool free_mode; //!< カーソルを任意のマスへ動かすモードか (false なら興味のあるものを順に選ぶモード)
    bool can_target; //!< 't' などでカーソルのマスをターゲットにできるか
    Pos2D pos; //!< カーソルのマスのフロアでの座標

    bool operator==(const TargetInputDetails &) const = default;
};

//! 入力の種類ごとの詳細。詳細を持たない入力では std::monostate
using InputDetails = std::variant<std::monostate, StringInputDetails, QuantityInputDetails, CommandInputDetails, ItemInputDetails, DirectionInputDetails, TargetInputDetails>;

/*!
 * @brief 現在待っている入力の種類と、画面に表示したプロンプト
 */
struct InputContext {
    InputKind kind; //!< 入力の種類
    std::string prompt; //!< 画面に表示したプロンプト (アイテムの選択では、画面の行のうち選ぶ目的の部分)
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
