#pragma once

#include "load/item/item-loader-base.h"

struct object_type;
class ItemLoader10 : public ItemLoaderBase {
public:
    ItemLoader10() = default;
    void rd_item(object_type *o_ptr) override;
};
