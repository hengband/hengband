/*!
 * @brief Finalizerクラスのテスト
 *
 * util/finalizer.h の Finalizer と make_finalizer を検証する。
 * ゲーム本体とテストの両方で状態の復元に使われているため、
 * 「スコープを抜けたときにちょうど1回だけ実行される」ことを様々な抜け方で確かめる。
 * コピー/ムーブの禁止、例外仕様、制約といったコンパイル時の性質は static_assert で検証する。
 */

#include "util/finalizer.h"

#include <doctest/doctest.h>

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

/*!
 * @brief 呼び出し・コピー・ムーブの回数を数える関数オブジェクト
 */
class CountingCallable {
public:
    int *calls;
    int *copies;
    int *moves;

    CountingCallable(int &calls, int &copies, int &moves)
        : calls(&calls)
        , copies(&copies)
        , moves(&moves)
    {
    }

    CountingCallable(const CountingCallable &other)
        : calls(other.calls)
        , copies(other.copies)
        , moves(other.moves)
    {
        ++*this->copies;
    }

    CountingCallable(CountingCallable &&other) noexcept
        : calls(other.calls)
        , copies(other.copies)
        , moves(other.moves)
    {
        ++*this->moves;
    }

    CountingCallable &operator=(const CountingCallable &) = default;
    CountingCallable &operator=(CountingCallable &&) = default;
    ~CountingCallable() = default;

    void operator()()
    {
        ++*this->calls;
    }
};

/*!
 * @brief コピーとムーブのどちらか一方だけが例外を投げうる関数オブジェクト
 * @tparam nothrow_copy true ならコピーが例外を投げずムーブが例外を投げうる。false ならその逆
 */
template <bool nothrow_copy>
class NoexceptSpecCallable {
public:
    NoexceptSpecCallable() = default;
    NoexceptSpecCallable(const NoexceptSpecCallable &) noexcept(nothrow_copy) {}
    NoexceptSpecCallable(NoexceptSpecCallable &&) noexcept(!nothrow_copy) {}
    NoexceptSpecCallable &operator=(const NoexceptSpecCallable &) = default;
    NoexceptSpecCallable &operator=(NoexceptSpecCallable &&) = default;
    ~NoexceptSpecCallable() = default;

    void operator()() {}
};

using ThrowingCopyCallable = NoexceptSpecCallable<false>;
using ThrowingMoveCallable = NoexceptSpecCallable<true>;

/*!
 * @brief コピーできずムーブだけできる関数オブジェクト
 */
class MoveOnlyCallable {
public:
    MoveOnlyCallable() = default;
    MoveOnlyCallable(const MoveOnlyCallable &) = delete;
    MoveOnlyCallable(MoveOnlyCallable &&) = default;
    MoveOnlyCallable &operator=(const MoveOnlyCallable &) = delete;
    MoveOnlyCallable &operator=(MoveOnlyCallable &&) = default;
    ~MoveOnlyCallable() = default;

    void operator()() {}
};

//! 呼び出せない型
struct NotInvocable {};

//! 右辺値としてしか呼び出せない関数オブジェクト (Finalizer は左辺値として呼び出すため受け付けない)
struct RvalueOnlyCallable {
    void operator()() && {}
};

template <typename T>
concept CanInstantiateFinalizer = requires { typename util::Finalizer<T>; };

template <typename T>
concept CanMakeFinalizer = requires(T &&func) { util::make_finalizer(std::forward<T>(func)); };

using FunctionPointerFinalizer = util::Finalizer<void (*)()>;

// コピーもムーブもできない
static_assert(!std::is_copy_constructible_v<FunctionPointerFinalizer>);
static_assert(!std::is_copy_assignable_v<FunctionPointerFinalizer>);
static_assert(!std::is_move_constructible_v<FunctionPointerFinalizer>);
static_assert(!std::is_move_assignable_v<FunctionPointerFinalizer>);

// 例外仕様は、左辺値ならコピー、右辺値ならムーブの例外仕様に従う
static_assert(!noexcept(util::make_finalizer(std::declval<ThrowingCopyCallable &>())));
static_assert(noexcept(util::make_finalizer(std::declval<ThrowingCopyCallable>())));
static_assert(noexcept(util::make_finalizer(std::declval<ThrowingMoveCallable &>())));
static_assert(!noexcept(util::make_finalizer(std::declval<ThrowingMoveCallable>())));

// 左辺値として引数なしで呼び出せる型だけを受け付ける
static_assert(CanInstantiateFinalizer<void (*)()>);
static_assert(!CanInstantiateFinalizer<NotInvocable>);
static_assert(!CanInstantiateFinalizer<RvalueOnlyCallable>);
static_assert(CanMakeFinalizer<void (*)()>);
static_assert(!CanMakeFinalizer<NotInvocable>);
static_assert(!CanMakeFinalizer<RvalueOnlyCallable>);

// 左辺値はコピー、右辺値はムーブして保持するため、コピーできない関数オブジェクトは右辺値でしか渡せない
static_assert(CanMakeFinalizer<MoveOnlyCallable>);
static_assert(!CanMakeFinalizer<MoveOnlyCallable &>);

// 参照型を保持すると元のオブジェクトの破棄後に呼び出してしまうため、オブジェクト型だけを受け付ける
static_assert(!CanInstantiateFinalizer<CountingCallable &>);
static_assert(!CanInstantiateFinalizer<CountingCallable &&>);
static_assert(!CanInstantiateFinalizer<void()>);

int function_pointer_calls = 0;

void increment_function_pointer_calls()
{
    ++function_pointer_calls;
}

}

TEST_CASE("Finalizer runs the function once when leaving the scope")
{
    auto calls = 0;
    {
        const auto finalizer = util::make_finalizer([&calls] { ++calls; });
        CHECK(calls == 0);
    }
    CHECK(calls == 1);
}

TEST_CASE("Finalizer runs the function during stack unwinding")
{
    auto calls = 0;
    CHECK_THROWS_AS([&calls] {
        const auto finalizer = util::make_finalizer([&calls] { ++calls; });
        throw std::runtime_error("test");
    }(),
        std::runtime_error);
    CHECK(calls == 1);
}

TEST_CASE("Finalizers run in reverse order of construction")
{
    std::vector<int> order;
    {
        const auto first = util::make_finalizer([&order] { order.push_back(1); });
        const auto second = util::make_finalizer([&order] { order.push_back(2); });
        const auto third = util::make_finalizer([&order] { order.push_back(3); });
    }
    const std::vector<int> expected{ 3, 2, 1 };
    CHECK(order == expected);
}

TEST_CASE("make_finalizer copies an lvalue and moves an rvalue")
{
    auto calls = 0;
    auto copies = 0;
    auto moves = 0;

    SUBCASE("lvalue is copied")
    {
        CountingCallable callable(calls, copies, moves);
        {
            const auto finalizer = util::make_finalizer(callable);
            CHECK(copies == 1);
            CHECK(moves == 0);
        }
        CHECK(calls == 1);
    }

    SUBCASE("rvalue is moved")
    {
        {
            const auto finalizer = util::make_finalizer(CountingCallable(calls, copies, moves));
            CHECK(copies == 0);
            CHECK(moves == 1);
        }
        CHECK(calls == 1);
    }
}

TEST_CASE("Finalizer accepts a function pointer")
{
    function_pointer_calls = 0;
    {
        const auto finalizer = util::make_finalizer(&increment_function_pointer_calls);
        CHECK(function_pointer_calls == 0);
    }
    CHECK(function_pointer_calls == 1);
}
