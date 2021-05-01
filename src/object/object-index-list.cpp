#include "object/object-index-list.h"

void ObjectIndexList::add(OBJECT_IDX o_idx)
{
    o_idx_list_.push_front(o_idx);
}

void ObjectIndexList::remove(OBJECT_IDX o_idx)
{
    o_idx_list_.remove(o_idx);
}

void ObjectIndexList::rotate()
{
    if (o_idx_list_.size() < 2)
        return;

    o_idx_list_.push_back(o_idx_list_.front());
    o_idx_list_.pop_front();
}
