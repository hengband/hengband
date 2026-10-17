/*!
 * @file input-context.cpp
 * @brief 入力待ちの文脈の実装
 */

#include "io/input-context.h"
#include <string>
#include <string_view>
#include <tl/optional.hpp>
#include <utility>

namespace {
tl::optional<InputContext> current_input_context;
}

/*!
 * @brief 新しい入力文脈を保持し、以前の文脈をスコープへ退避する
 * @details 新しい値の構築が失敗した場合は、現在の文脈を変更しない。
 */
InputContextScope::InputContextScope(InputKind kind, std::string_view prompt)
    : InputContextScope(InputContext{ kind, std::string(prompt), {} })
{
}

/*!
 * @brief 詳細情報を含む入力文脈を保持し、以前の文脈を退避する
 * @details 新しい値の構築が失敗した場合は、現在の文脈を変更しない。
 */
InputContextScope::InputContextScope(InputContext input_context)
    : previous(std::move(input_context))
{
    current_input_context.swap(this->previous);
}

/*!
 * @brief 退避していた入力文脈を復元する
 */
InputContextScope::~InputContextScope()
{
    current_input_context.swap(this->previous);
}

/*!
 * @brief 現在の入力文脈を返す。入力文脈がなければ空のoptionalを返す。
 */
const tl::optional<InputContext> &get_input_context()
{
    return current_input_context;
}
