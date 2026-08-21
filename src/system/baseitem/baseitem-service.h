#pragma once

#include "util/flag-group.h"
#include <vector>

enum class BaseitemCollectionMode {
    CHECK_CHANCE, //!< グループに該当アイテムが1つでもあるかを調べ、最初の1件で打ち切る
    VISUAL_ONLY, //!< 鑑定状態と出現率を無視して収集する
    MAX,
};

enum class ItemKindType : short;
class BaseitemDefinition;
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
    static BaseitemConfig &get_flavor_config(short bi_id);
    static std::vector<short> collect_baseitem_ids(int grp_cur, const EnumClassFlagGroup<BaseitemCollectionMode> &mode);

private:
    static void shuffle_flavors(ItemKindType tval);
    static bool check_chance(const EnumClassFlagGroup<BaseitemCollectionMode> &mode, const BaseitemDefinition &baseitem, const BaseitemRecord &record);
};
