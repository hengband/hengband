/*!
 * @brief オブジェクトに関する汎用判定処理
 * @date 2018/09/24
 * @author deskull
 */

#include "object/item-tester-hooker.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-describer.h"

/**
 * @brief Construct a new Tval Item Tester:: Tval Item Tester object
 *
 * @param tval テストOKとなるtvalを指定する
 */
TvalItemTester::TvalItemTester(tval_type tval)
    : tval(tval)
{
}

/**
 * @brief Construct a new Func Item Tester:: Func Item Tester object
 *
 * @param test_func そのオブジェクトが条件に合うならtrueを返すメンバ関数を指定する
 */
FuncItemTester::FuncItemTester(TestMemberFunctionPtr test_func)
    : test_func([f = test_func](player_type *, const object_type *o_ptr) { return (o_ptr->*f)(); })
{
}

/**
 * @brief Construct a new Func Item Tester:: Func Item Tester object
 *
 * @param test_func 引数に object_type へのポインタを取り、そのオブジェクトが条件に合うならtrueを返す関数を指定する
 */
FuncItemTester::FuncItemTester(std::function<bool(const object_type *)> test_func)
    : test_func([f = std::move(test_func)](player_type *, const object_type *o_ptr) { return f(o_ptr); })
{
}

/*!
 * @brief Construct a new Func Item Tester:: Func Item Tester object
 *
 * @param test_func 引数に player_type へのポインタと object_type へのポインタを取り、そのオブジェクトが条件に合うならtrueを返す関数を指定する
 * @param player_ptr test_func の player_type へのポインタの引数に対して渡すポインタを指定する
 */
FuncItemTester::FuncItemTester(std::function<bool(player_type *, const object_type *)> test_func, player_type *player_ptr)
    : test_func(std::move(test_func))
    , player_ptr(player_ptr)
{
}

/*!
 * @brief アイテムが条件を満たしているか調べる
 * Check an item against the item tester info
 * @param o_ptr 判定を行いたいオブジェクト構造体参照ポインタ
 * @return アイテムが条件を満たしているならtrueを返す
 * @details 最初にk_idxが無効でないか等の共通の判定を行った後に子クラスで実装される okay_impl 関数の結果を返す
 */
bool ItemTester::okay(const object_type *o_ptr) const
{
    if (o_ptr->k_idx == 0)
        return false;

    if (o_ptr->tval == TV_GOLD) {
        if (!show_gold_on_floor)
            return false;
    }

    return this->okay_impl(o_ptr);
}

bool TvalItemTester::okay_impl(const object_type *o_ptr) const
{
    return this->tval == o_ptr->tval;
}

bool FuncItemTester::okay_impl(const object_type *o_ptr) const
{
    return this->test_func(this->player_ptr, o_ptr);
}
