#pragma once

#include "system/angband.h"

#include <list>

class FloorType;

/**
 * @brief アイテムリスト(床上スタック/モンスター所持)を管理するクラス
 *
 * @details ObjectType 自体を保持するのではなく、フロア全体の ObjectType 配列上のアイテムの要素番号を保持する
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
     * @details
     * stack_idx が非負数の場合、アイテムリストの stack_idx で指定した位置に追加する。
     * (リストに含まれるアイテムの stack_idx は降順で並んでいるものと想定し、その順序を崩さない位置に追加する)
     *
     * stack_idx が0または負数の場合、アイテムリストの先頭に追加する。
     * また、追加したアイテムの stack_idx を追加前のアイテムリストの先頭のアイテムの stack_idx + 1 にする。
     *
     * @param floor_ptr 追加するアイテムが存在するフロアへのポインタ
     * @param o_idx 追加するアイテムのフロア全体のアイテム配列上の要素番号
     * @param stack_idx アイテムリストに追加する位置(デフォルト:0)
     */
    void add(FloorType *floor_ptr, OBJECT_IDX o_idx, IDX stack_idx = 0);

    /**
     * @brief アイテムリストからフロア全体のアイテム配列上の指定した要素番号のアイテムを削除する
     *
     * @param o_idx 削除するアイテムのフロア全体のアイテム配列上の要素番号
     */
    void remove(OBJECT_IDX o_idx);

    /**
     * @brief アイテムリストの先頭のアイテムを最後尾に移動させる
     *
     * @details
     * アイテムリストに含まれる各アイテムの stack_idx の降順を崩さないようにするため、
     * 先頭のアイテムを最後尾に移動した後各アイテムの stack_idx は1ずつインクリメントされ、
     * 最後尾に移動したアイテムの stack_idx は 1 になる。
     */
    void rotate(FloorType *floor_ptr);

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
