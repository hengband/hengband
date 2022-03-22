#pragma once

#include "object/tval-types.h"

#include <functional>
#include <memory>

enum class StoreSaleType;
class ObjectType;
class PlayerType;

/*!
 * @brief アイテムの絞り込み条件をテストする基底クラス
 */
class ItemTester {
public:
    virtual ~ItemTester() = default;
    bool okay(const ObjectType *o_ptr) const;
    virtual std::unique_ptr<ItemTester> clone() const = 0;

protected:
    ItemTester() = default;

private:
    virtual bool okay_impl(const ObjectType *o_ptr) const = 0;
};

/**
 * @brief ItemTesterの派生クラスのオブジェクトを複製するメンバ関数 clone() を実装するクラス
 *
 * @tparam T ItemTesterの派生クラスのオブジェクトの型
 */
template <typename DerivedItemTester>
class CloneableItemTester : public ItemTester {
public:
    virtual std::unique_ptr<ItemTester> clone() const final
    {
        return std::make_unique<DerivedItemTester>(static_cast<const DerivedItemTester &>(*this));
    }
};

/*!
 * @brief 全てのアイテムがOKとなるアイテムテストクラス
 */
class AllMatchItemTester : public CloneableItemTester<AllMatchItemTester> {
public:
    AllMatchItemTester() = default;

private:
    virtual bool okay_impl(const ObjectType *) const
    {
        return true;
    }
};

/*!
 * @brief 指定した tval のアイテムならばOKとなるアイテムテストクラス
 */
class TvalItemTester : public CloneableItemTester<TvalItemTester> {
public:
    explicit TvalItemTester(ItemKindType tval);

private:
    virtual bool okay_impl(const ObjectType *o_ptr) const;

    ItemKindType tval;
};

/*!
 * @brief 指定した関数を呼び出した結果戻り値が true であればOKとなるアイテムテストクラス
 */
class FuncItemTester : public CloneableItemTester<FuncItemTester> {
public:
    using TestMemberFunctionPtr = bool (ObjectType::*)() const;
    explicit FuncItemTester(TestMemberFunctionPtr test_func);
    explicit FuncItemTester(std::function<bool(const ObjectType *)> test_func);
    explicit FuncItemTester(std::function<bool(PlayerType *, const ObjectType *)> test_func, PlayerType *player_ptr);
    explicit FuncItemTester(std::function<bool(PlayerType *, const ObjectType *, StoreSaleType)> test_func, PlayerType *player_ptr, StoreSaleType store_num);

private:
    virtual bool okay_impl(const ObjectType *o_ptr) const;

    std::function<bool(PlayerType *, const ObjectType *)> test_func;
    PlayerType *player_ptr;
};
