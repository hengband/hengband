#include "system/baseitem/baseitem-service.h"
#include "system/baseitem/baseitem-config.h"
#include "system/baseitem/baseitem-configs.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "term/z-rand.h"
#include "view/display-symbol.h"
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view.hpp>

void BaseitemService::initialize_baseitem_configs()
{
    const auto &baseitems = BaseitemList::get_instance();
    auto &baseitem_configs = BaseitemConfigs::get_instance();
    const auto range = ranges::views::iota(0) | ranges::views::take(baseitems.size());
    ranges::for_each(range, [&baseitems, &baseitem_configs](short bi_id) {
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        baseitem_configs.emplace_back(baseitem.get_symbol());
    });
}

void BaseitemService::reset_all_visuals()
{
    const auto &baseitems = BaseitemList::get_instance();
    auto &baseitem_configs = BaseitemConfigs::get_instance();
    const auto range = ranges::views::iota(0) | ranges::views::take(baseitems.size());
    ranges::for_each(range, [&baseitems, &baseitem_configs](short bi_id) {
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        baseitem_configs.set_config(bi_id, baseitem.get_symbol());
    });
}

const BaseitemConfig &BaseitemService::pick_one_at_random()
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem_configs = BaseitemConfigs::get_instance();
    while (true) {
        const auto bi_id = randnum1<short>(baseitems.size() - 1); // 0は無効値なので最初から選ばない
        if (baseitems.is_valid(bi_id)) {
            return baseitem_configs.get_config(bi_id);
        }
    }
}

const DisplaySymbol &BaseitemService::get_dummy_symbol()
{
    static const auto ds = BaseitemConfigs::get_instance().get_config(0).get_symbol();
    return ds;
}
