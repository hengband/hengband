#pragma once

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace util {

/*!
 * @brief オブジェクトがスコープを抜ける際に、コンストラクタで渡された関数を実行するクラス
 *
 * コード例:
 * @code
 * void func()
 * {
 *     auto finalizer = util::make_finalizer([] { std::cout << "finally" << std::endl; });
 *     std::cout << "do something" << std::endl;
 * }
 * @endcode
 * この場合、func()が終了する際に"finally"と表示される。
 *
 * @param func 実行する関数
 */
template <std::invocable Func>
class Finalizer {
public:
    explicit Finalizer(const Func &func) noexcept
        : func_{ func }
    {
    }
    explicit Finalizer(Func &&func) noexcept
        : func_{ std::move(func) }
    {
    }

    ~Finalizer() noexcept
    {
        std::invoke(func_);
    }

    Finalizer(const Finalizer &) = delete;
    void operator=(const Finalizer &) = delete;
    Finalizer(Finalizer &&) = delete;
    void operator=(Finalizer &&) = delete;

private:
    Func func_;
};

/*!
 * @brief Finalizerオブジェクトを生成するファクトリ関数
 * @param func Finalizerオブジェクトのコンストラクタに渡す関数
 */
template <typename Func>
[[nodiscard]] auto make_finalizer(Func &&func) noexcept
{
    return Finalizer<std::decay_t<Func>>{ std::forward<Func>(func) };
}

}
