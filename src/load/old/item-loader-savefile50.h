#pragma once

#include "load/item/item-loader-base.h"

class ItemEntity;
class ItemLoader50 : public ItemLoaderBase {
public:
    ItemLoader50() = default;
    void rd_item(ItemEntity *o_ptr) override;
};
