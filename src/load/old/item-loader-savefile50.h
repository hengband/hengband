#pragma once

#include "load/item/item-loader-base.h"

class ObjectType;
class ItemLoader50 : public ItemLoaderBase {
public:
    ItemLoader50() = default;
    void rd_item(ObjectType *o_ptr) override;
};
