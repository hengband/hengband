/*!
 * @file input-context.cpp
 * @brief ゲームが待っているキー入力の種類の管理
 */

#include "io/input-context.h"

namespace {
tl::optional<InputContextKind> current_input_context; //!< 現在有効なキー入力の種類
}

/*!
 * @brief キー入力の種類を設定する
 * @param kind 設定する種類
 */
InputContextGuard::InputContextGuard(InputContextKind kind)
    : previous(current_input_context)
{
    current_input_context = kind;
}

/*!
 * @brief 設定する前のキー入力の種類に戻す
 */
InputContextGuard::~InputContextGuard()
{
    current_input_context = this->previous;
}

/*!
 * @brief 現在有効なキー入力の種類を取得する
 * @return 現在有効な種類。どの入力待ちの中でもなければnullopt
 */
tl::optional<InputContextKind> get_input_context()
{
    return current_input_context;
}
