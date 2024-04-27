#pragma once

class ItemEntity;
class ItemLoaderBase {
public:
    virtual ~ItemLoaderBase() = default;

    virtual void rd_item(ItemEntity *o_ptr) = 0;
    void load_item();
    void load_artifact();

protected:
    ItemLoaderBase() = default;
};
