#pragma once

#include <vector>

enum class ItemKindType : short;
class BaseitemConfig;
class BaseitemRecord;
class DisplaySymbol;
class BaseitemService {
public:
    BaseitemService() = delete;

    static void initialize_baseitem_records();
    static void initialize_baseitem_configs();
    static void reset_all_visuals();
    static const BaseitemConfig &pick_one_at_random();
    static const DisplaySymbol &get_dummy_symbol();
    static void shuffle_flavors();
    static void mark_common_items_as_aware();
    static void initialize_items_flavor();

private:
    static void shuffle_flavors(ItemKindType tval);
};
