#include "view/display-lore-drops.h"
#include "lore/lore-util.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <fmt/format.h>

void display_monster_drop_quantity(lore_type *lore_ptr)
{
    lore_ptr->drop_quantity = std::max(lore_ptr->drop_gold, lore_ptr->drop_item);
    if (lore_ptr->drop_quantity == 1) {
        hooked_roff(_("一つの", " a"));
#ifdef JP
#else
        lore_ptr->sin = true;
#endif
    } else if (lore_ptr->drop_quantity == 2) {
        hooked_roff(_("一つか二つの", " one or two"));
    } else {
        hooked_roff(fmt::format(_(" {} 個までの", " up to {}"), lore_ptr->drop_quantity));
    }
}

void display_monster_drop_quality(lore_type *lore_ptr)
{
    if (lore_ptr->drop_flags.has(MonsterDropType::DROP_GREAT)) {
        lore_ptr->drop_quality = _("特別な", " exceptional");
    } else if (lore_ptr->drop_flags.has(MonsterDropType::DROP_GOOD)) {
        lore_ptr->drop_quality = _("上質な", " good");
#ifdef JP
#else
        lore_ptr->sin = false;
#endif
    } else {
        lore_ptr->drop_quality = nullptr;
    }
}

void display_monster_drop_items(lore_type *lore_ptr)
{
    if (lore_ptr->drop_item == 0) {
        return;
    }

#ifdef JP
#else
    if (lore_ptr->sin) {
        hooked_roff("n");
    }

    lore_ptr->sin = false;
#endif

    if (lore_ptr->drop_quality != nullptr) {
        hooked_roff(lore_ptr->drop_quality);
    }

    hooked_roff(_("アイテム", " object"));
#ifdef JP
#else
    if (lore_ptr->drop_quantity != 1) {
        hooked_roff("s");
    }
#endif
    lore_ptr->drop_quality = _("や", " or");
}

void display_monster_drop_golds(lore_type *lore_ptr)
{
    auto is_item_only = lore_ptr->drop_gold == 0;
    is_item_only |= lore_ptr->drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT });
    if (is_item_only) {
        return;
    }

#ifdef JP
#else
    if (lore_ptr->drop_quality == nullptr) {
        lore_ptr->sin = false;
    }

    if (lore_ptr->sin) {
        hooked_roff("n");
    }

    lore_ptr->sin = false;
#endif

    if (lore_ptr->drop_quality != nullptr) {
        hooked_roff(lore_ptr->drop_quality);
    }

    hooked_roff(_("財宝", " treasure"));
#ifdef JP
#else
    if (lore_ptr->drop_quantity != 1) {
        hooked_roff("s");
    }
#endif
}

void display_monster_drops_summary(lore_type *lore_ptr)
{
    if ((lore_ptr->drop_gold == 0) && (lore_ptr->drop_item == 0)) {
        return;
    }
    const auto drop_num = std::max(lore_ptr->drop_gold, lore_ptr->drop_item);
    std::string drop_what;
    std::string drop_quality;

    if (lore_ptr->drop_flags.has(MonsterDropType::DROP_GREAT)) {
        drop_quality = _("特別 ", "great ");
    } else if (lore_ptr->drop_flags.has(MonsterDropType::DROP_GOOD)) {
        drop_quality = _("上質 ", "good ");
    }

    auto is_item_only = lore_ptr->drop_gold == 0;
    is_item_only |= lore_ptr->drop_flags.has_any_of({ MonsterDropType::DROP_GOOD, MonsterDropType::DROP_GREAT });
    if (is_item_only && lore_ptr->drop_item > 0) {
        drop_what = _("アイテム", "item");
    }
    if (!is_item_only && lore_ptr->drop_item > 0 && lore_ptr->drop_gold > 0) {
        drop_what = _("アイテム/財宝", "item/treasure");
    }
    if (!is_item_only && lore_ptr->drop_item == 0 && lore_ptr->drop_gold > 0) {
        drop_what = _("財宝", "treasure");
    }

    hooked_roff(fmt::format(_("[ドロップ] {}({}最大{})\n", "[drop] {}({}max {})\n"), drop_what, drop_quality, drop_num));
}

void display_monster_drops(lore_type *lore_ptr)
{
    if ((lore_ptr->drop_gold == 0) && (lore_ptr->drop_item == 0)) {
        return;
    }

    hooked_roff(fmt::format(_("{}は", "{} may carry"), str_upcase_first(Who::who(lore_ptr->msex))));
#ifdef JP
#else
    lore_ptr->sin = false;
#endif

    display_monster_drop_quantity(lore_ptr);
    display_monster_drop_quality(lore_ptr);
    display_monster_drop_items(lore_ptr);
    display_monster_drop_golds(lore_ptr);
    hooked_roff(_("を持っていることがある。", ".  "));
}
