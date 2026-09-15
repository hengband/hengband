#pragma once

#include <concepts>
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
template <typename Func>
    requires std::is_object_v<Func> && std::invocable<Func &>
class [[nodiscard]] Finalizer {
public:
    template <typename F>
        requires std::constructible_from<Func, F>
    explicit Finalizer(F &&func) noexcept(std::is_nothrow_constructible_v<Func, F>)
        : func_(std::forward<F>(func))
    {
    }

    ~Finalizer() noexcept
    {
        func_();
    }

    Finalizer(const Finalizer &) = delete;
    Finalizer &operator=(const Finalizer &) = delete;
    Finalizer(Finalizer &&) = delete;
    Finalizer &operator=(Finalizer &&) = delete;

private:
    Func func_;
};

/*!
 * @brief Finalizerオブジェクトを生成するファクトリ関数
 * @param func Finalizerオブジェクトのコンストラクタに渡す関数
 */
template <typename Func>
    requires std::invocable<std::decay_t<Func> &> && std::constructible_from<std::decay_t<Func>, Func>
[[nodiscard]] auto make_finalizer(Func &&func) noexcept(std::is_nothrow_constructible_v<Finalizer<std::decay_t<Func>>, Func>)
{
    return Finalizer<std::decay_t<Func>>{ std::forward<Func>(func) };
}

}
