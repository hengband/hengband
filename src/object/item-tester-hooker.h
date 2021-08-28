#pragma once

#include "object/tval-types.h"

#include <functional>
#include <memory>

struct object_type;
struct player_type;

/*!
 * @brief アイテムの絞り込み条件をテストする基底クラス
 */
class ItemTester {
public:
    virtual ~ItemTester() = default;
    bool okay(const object_type *o_ptr) const;
    virtual std::unique_ptr<ItemTester> clone() const = 0;

protected:
    ItemTester() = default;

private:
    virtual bool okay_impl(const object_type *o_ptr) const = 0;
};

/*!
 * @brief 全てのアイテムがOKとなるアイテムテストクラス
 */
class AllMatchItemTester : public ItemTester {
public:
    AllMatchItemTester() = default;

    virtual std::unique_ptr<ItemTester> clone() const
    {
        return std::make_unique<AllMatchItemTester>(*this);
    }

private:
    virtual bool okay_impl(const object_type *) const
    {
        return true;
    }
};

/*!
 * @brief 指定した tval のアイテムならばOKとなるアイテムテストクラス
 */
class TvalItemTester : public ItemTester {
public:
    explicit TvalItemTester(tval_type tval);

    virtual std::unique_ptr<ItemTester> clone() const
    {
        return std::make_unique<TvalItemTester>(*this);
    }

private:
    virtual bool okay_impl(const object_type *o_ptr) const;

    tval_type tval;
};

/*!
 * @brief 指定した関数を呼び出した結果戻り値が true であればOKとなるアイテムテストクラス
 */
class FuncItemTester : public ItemTester {
public:
    explicit FuncItemTester(std::function<bool(const object_type *)> test_func);
    explicit FuncItemTester(std::function<bool(player_type *, const object_type *)> test_func, player_type *player_ptr);

    virtual std::unique_ptr<ItemTester> clone() const
    {
        return std::make_unique<FuncItemTester>(*this);
    }

private:
    virtual bool okay_impl(const object_type *o_ptr) const;

    std::function<bool(player_type *, const object_type *)> test_func;
    player_type *player_ptr;
};
