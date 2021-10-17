#pragma once

#include "system/object-type-definition.h"

class ItemLoaderBase {
public:
    virtual ~ItemLoaderBase() = default;

    virtual void rd_item(object_type *o_ptr) = 0;
    void load_item(void);
    void load_artifact(void);

protected:
    ItemLoaderBase() = default;
};
