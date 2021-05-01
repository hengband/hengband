#pragma once

#include "system/angband.h"

#include <list>

/**
 * @brief アイテムリスト(床上スタック/モンスター所持)を管理するクラス
 */
class ObjectIndexList {
public:
    /**
     * @brief デフォルトコンストラクタ
     */
    ObjectIndexList() = default;

    /**
     * @brief アイテムリストにフロア全体のアイテム配列上の指定した要素番号のアイテムを追加する
     *
     * @param o_idx 追加するアイテムのフロア全体のアイテム配列上の要素番号
     */
    void add(OBJECT_IDX o_idx);

    /**
     * @brief アイテムリストからフロア全体のアイテム配列上の指定した要素番号のアイテムを削除する
     *
     * @param o_idx 削除するアイテムのフロア全体のアイテム配列上の要素番号
     */
    void remove(OBJECT_IDX o_idx);

    /**
     * @brief アイテムリストの先頭のアイテムを最後尾に移動させる
     */
    void rotate();

    //
    // 以下のメソッドは内部で保持している std::list オブジェクトに対して使用できる同名のメソッド
    //
    auto empty() const noexcept
    {
        return o_idx_list_.empty();
    }
    auto size() const noexcept
    {
        return o_idx_list_.size();
    }
    void clear() noexcept
    {
        o_idx_list_.clear();
    }
    auto &front() noexcept
    {
        return o_idx_list_.front();
    }
    void pop_front() noexcept
    {
        return o_idx_list_.pop_front();
    }
    auto begin() noexcept
    {
        return o_idx_list_.begin();
    }
    auto end() noexcept
    {
        return o_idx_list_.end();
    }
    auto begin() const noexcept
    {
        return o_idx_list_.begin();
    }
    auto end() const noexcept
    {
        return o_idx_list_.end();
    }

private:
    std::list<OBJECT_IDX> o_idx_list_;
};
