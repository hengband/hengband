#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

namespace util {

/*!
 * @brief オブジェクトがスコープを抜ける際に、コンストラクタで渡された関数を実行するクラス
 * @tparam Func 実行する関数オブジェクトの型。参照型でないオブジェクト型で、左辺値として引数なしで呼び出せること
 * @details 一時的に書き換えた状態を、スコープを抜けるときに元へ戻す用途を想定している。
 * 通常は直接構築せず、make_finalizer() で生成する。
 *
 * - 登録した関数は、デストラクタで1回だけ呼ばれる。
 * - 例外によってスコープを抜けた場合 (スタックの巻き戻し中) にも呼ばれる。
 * - 同じスコープに複数のファイナライザがある場合は、生成と逆の順序で呼ばれる。
 * - 登録した関数が例外を投げると、デストラクタが noexcept であるため std::terminate が呼ばれる。
 * - 関数が2回以上呼ばれることを防ぐため、コピーもムーブもできない。
 *
 * コード例:
 * @code
 * void func(uint32_t seed)
 * {
 *     auto &system = AngbandSystem::get_instance();
 *     const auto restore_rng = util::make_finalizer([&system, rng_backup = system.get_rng()] { system.set_rng(rng_backup); });
 *     Rand_state_init(seed);
 *     // 乱数生成器の状態を書き換えて何かをする
 * }
 * @endcode
 * この場合、func() から抜ける際に (例外で抜けた場合も含めて) 乱数生成器の状態が元に戻る。
 */
template <typename Func>
    requires std::is_object_v<Func> && std::invocable<Func &>
class [[nodiscard]] Finalizer {
public:
    /*!
     * @brief コンストラクタ
     * @tparam F 引数の型
     * @param func スコープを抜ける際に実行する関数オブジェクト。左辺値ならコピー、右辺値ならムーブして保持する
     */
    template <typename F>
        requires std::constructible_from<Func, F>
    explicit Finalizer(F &&func) noexcept(std::is_nothrow_constructible_v<Func, F>)
        : func_(std::forward<F>(func))
    {
    }

    /*!
     * @brief 保持している関数オブジェクトを実行する
     */
    ~Finalizer() noexcept
    {
        func_();
    }

    Finalizer(const Finalizer &) = delete;
    Finalizer &operator=(const Finalizer &) = delete;
    Finalizer(Finalizer &&) = delete;
    Finalizer &operator=(Finalizer &&) = delete;

private:
    Func func_; //!< スコープを抜ける際に実行する関数オブジェクト
};

/*!
 * @brief Finalizerオブジェクトを生成するファクトリ関数
 * @tparam Func 実行する関数オブジェクトの型 (ラムダ式、関数ポインタなど)
 * @param func スコープを抜ける際に実行する関数オブジェクト。左辺値ならコピー、右辺値ならムーブして保持する (そのため左辺値で渡す場合はコピーできること)
 * @return func を保持するFinalizerオブジェクト
 * @details 戻り値は必ず変数で受けること。受けないと一時オブジェクトがその場で破棄され、
 * 直ちに func が実行されてしまう (受け損ねた場合は [[nodiscard]] により警告が出る)。
 * Finalizer はムーブできないが、戻り値はコピー省略により呼び出し側の変数へ直接構築される。
 */
template <typename Func>
    requires std::invocable<std::decay_t<Func> &> && std::constructible_from<std::decay_t<Func>, Func>
[[nodiscard]] auto make_finalizer(Func &&func) noexcept(std::is_nothrow_constructible_v<Finalizer<std::decay_t<Func>>, Func>)
{
    return Finalizer<std::decay_t<Func>>{ std::forward<Func>(func) };
}

}
