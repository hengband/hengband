#include "object/object-index-list.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"

#include <algorithm>

void ObjectIndexList::add(FloorType *floor_ptr, OBJECT_IDX o_idx, IDX stack_idx)
{
    if (stack_idx <= 0) {
        stack_idx = o_idx_list_.empty() ? 1 : floor_ptr->o_list[o_idx_list_.front()].stack_idx + 1;
    }

    auto it = std::partition_point(
        o_idx_list_.begin(), o_idx_list_.end(), [floor_ptr, stack_idx](IDX idx) { return floor_ptr->o_list[idx].stack_idx > stack_idx; });

    o_idx_list_.insert(it, o_idx);
    floor_ptr->o_list[o_idx].stack_idx = stack_idx;
}

void ObjectIndexList::remove(OBJECT_IDX o_idx)
{
    o_idx_list_.remove(o_idx);
}

void ObjectIndexList::rotate(FloorType *floor_ptr)
{
    if (o_idx_list_.size() < 2) {
        return;
    }

    o_idx_list_.push_back(o_idx_list_.front());
    o_idx_list_.pop_front();

    for (const auto o_idx : o_idx_list_) {
        floor_ptr->o_list[o_idx].stack_idx++;
    }

    floor_ptr->o_list[o_idx_list_.back()].stack_idx = 1;
}
