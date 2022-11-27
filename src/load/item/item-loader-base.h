#pragma once

class ItemEntity;
class ItemLoaderBase {
public:
    virtual ~ItemLoaderBase() = default;

    virtual void rd_item(ItemEntity *o_ptr) = 0;
    void load_item(void);
    void load_artifact(void);

protected:
    ItemLoaderBase() = default;
};
