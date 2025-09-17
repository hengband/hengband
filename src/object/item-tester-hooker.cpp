/*!
 * @brief オブジェクトに関する汎用判定処理
 * @date 2018/09/24
 * @author deskull
 */

#include "object/item-tester-hooker.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "target/target-describer.h"

/**
 * @brief Construct a new Tval Item Tester:: Tval Item Tester object
 *
 * @param tval テストOKとなるtvalを指定する
 */
TvalItemTester::TvalItemTester(ItemKindType tval)
    : tval(tval)
{
}

/**
 * @brief Construct a new Func Item Tester:: Func Item Tester object
 *
 * @param test_func 引数に ItemEntity へのポインタを取り、そのオブジェクトが条件に合うならtrueを返す関数を指定する
 */
FuncItemTester::FuncItemTester(std::function<bool(const ItemEntity &)> test_func)
    : test_func([f = std::move(test_func)](PlayerType *, const ItemEntity &item) { return f(item); })
{
}

/*!
 * @brief Construct a new Func Item Tester:: Func Item Tester object
 *
 * @param test_func 引数に PlayerType へのポインタと ItemEntity へのポインタを取り、そのオブジェクトが条件に合うならtrueを返す関数を指定する
 * @param player_ptr test_func の PlayerType へのポインタの引数に対して渡すポインタを指定する
 */
FuncItemTester::FuncItemTester(std::function<bool(PlayerType *, const ItemEntity &)> test_func, PlayerType *player_ptr)
    : test_func(std::move(test_func))
    , player_ptr(player_ptr)
{
}

/*!
 * @brief Construct a new Func Item Tester:: Func Item Tester object
 *
 * @param test_func 引数に PlayerType へのポインタと ItemEntity へのポインタと StoreSaleType を取り、そのオブジェクトが条件に合うならtrueを返す関数を指定する
 * @param player_ptr test_func の PlayerType へのポインタの引数に対して渡すポインタを指定する
 */
FuncItemTester::FuncItemTester(std::function<bool(PlayerType *, const ItemEntity &, StoreSaleType)> test_func, PlayerType *player_ptr, StoreSaleType store_num)
    : test_func([test_func = std::move(test_func), store_num](PlayerType *player_ptr, const ItemEntity &item) { return test_func(player_ptr, item, store_num); })
    , player_ptr(player_ptr)
{
}

/*!
 * @brief アイテムが条件を満たしているか調べる
 * Check an item against the item tester info
 * @param item 判定を行いたいアイテムへの参照
 * @return アイテムが条件を満たしているならtrueを返す
 * @details 最初にbi_idが無効でないか等の共通の判定を行った後に子クラスで実装される okay_impl 関数の結果を返す
 */
bool ItemTester::okay(const ItemEntity &item) const
{
    if (!item.is_valid()) {
        return false;
    }

    if (item.bi_key.tval() == ItemKindType::GOLD) {
        if (!show_gold_on_floor) {
            return false;
        }
    }

    return this->okay_impl(item);
}

bool TvalItemTester::okay_impl(const ItemEntity &item) const
{
    return this->tval == item.bi_key.tval();
}

bool FuncItemTester::okay_impl(const ItemEntity &item) const
{
    return this->test_func(this->player_ptr, item);
}
