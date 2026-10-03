#pragma once

#include "util/finalizer.h"

#include <tuple>

namespace test {

/*!
 * @brief 変数の値を退避し、スコープを抜けるときに元の値へ戻す
 * @param vars 退避する変数 (コピー代入できるもの)
 * @return スコープを抜けるときに変数を元の値へ戻すファイナライザ
 * @details テストで書き換えるグローバル変数やシングルトンのメンバを、テストの後に元へ戻すために使う。
 *          戻り値は必ず変数で受けること。受けないとその場で復元されてしまう。
 *          受け損ねた場合は [[nodiscard]] により警告が出る
 *          (警告をエラーとして扱う CI や Visual Studio のビルドでは失敗する)。
 *          配列のようにコピー代入できないものや、関数を呼んで戻すものは util::make_finalizer() を直接使う。
 */
template <typename... Ts>
[[nodiscard]] auto scoped_restore(Ts &...vars)
{
    return util::make_finalizer([&vars..., saved = std::tuple<Ts...>(vars...)] { std::tie(vars...) = saved; });
}

}
