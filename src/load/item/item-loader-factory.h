#pragma once

#include <memory>

enum class ItemLoaderVersionType;
class ItemLoaderBase;
class ItemLoaderFactory {
public:
    static std::shared_ptr<ItemLoaderBase> get_item_loader();

private:
    ItemLoaderFactory() = delete;
    static ItemLoaderVersionType get_version();
};
