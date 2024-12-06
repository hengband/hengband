#include "floor/floor-list.h"
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
 */
FloorType &FloorList::get_floor(int num)
{
    (void)num;

    return this->floor;
}
