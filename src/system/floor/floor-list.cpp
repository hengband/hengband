#include "system/floor/floor-list.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"

FloorList FloorList::instance{};

FloorList &FloorList::get_instance()
{
    return instance;
}

/*!
 * @brief 指定IDからフロアの参照を取得する
 * @param num 番号（現状は今後配列化する際の仮定義）
 * @return フロアの参照
 * @details 現状num はどの数値を指定しても一緒だが、置換しやすいように0を使うこと
 */
FloorType &FloorList::get_floor(int num)
{
    (void)num;

    return this->floor;
}
