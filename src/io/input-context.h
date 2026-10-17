#pragma once

#include <tl/optional.hpp>

/*!
 * @brief ゲームが待っているキー入力の種類
 * @details 制御サーバのクライアントが、今どのような入力を求められているかを知るためのもの。
 */
enum class InputContextKind {
    COMMAND, //!< コマンドの入力
    STORE_COMMAND, //!< 店内でのコマンドの入力
    YES_NO, //!< y/nの確認
    STRING, //!< 文字列・数値の入力
    DIRECTION, //!< 方向の入力
    TARGET, //!< ターゲットの選択 (見回しを含む)
    ITEM, //!< アイテムの選択
    ANY_KEY, //!< 何かキーを押すまでの一時停止
    BIRTH, //!< キャラクターの作成
    MENU, //!< 画面に並んだ選択肢からの選択や、一覧の閲覧 (帰還先のダンジョン、メッセージの履歴等)
    EDITOR, //!< テキストエディタ (自動拾いエディタ)。文字は入力され、ESCではメニューが開く
};

/*!
 * @brief 生存期間の間、ゲームが待っているキー入力の種類を設定する
 * @details 入れ子になった場合は最も内側のものが有効になり、破棄されると外側のものに戻る。
 */
class InputContextGuard {
public:
    explicit InputContextGuard(InputContextKind kind);
    ~InputContextGuard();
    InputContextGuard(const InputContextGuard &) = delete;
    InputContextGuard &operator=(const InputContextGuard &) = delete;
    InputContextGuard(InputContextGuard &&) = delete;
    InputContextGuard &operator=(InputContextGuard &&) = delete;

private:
    tl::optional<InputContextKind> previous; //!< 設定する前の種類
};

tl::optional<InputContextKind> get_input_context();
