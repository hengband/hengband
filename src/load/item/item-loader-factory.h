#pragma once

#include <memory>

enum class ItemLoaderVersionType;
class ItemLoaderBase;
class ItemLoaderFactory {
public:
    static std::unique_ptr<ItemLoaderBase> create_loader();

private:
    ItemLoaderFactory() = delete;
    static ItemLoaderVersionType get_version();
};
