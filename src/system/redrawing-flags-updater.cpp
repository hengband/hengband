#include "system/redrawing-flags-updater.h"
#include "util/enum-range.h"

RedrawingFlagsUpdater RedrawingFlagsUpdater::instance{};

RedrawingFlagsUpdater &RedrawingFlagsUpdater::get_instance()
{
    return instance;
}

bool RedrawingFlagsUpdater::any_main() const
{
    return this->main_window_flags.any();
}

bool RedrawingFlagsUpdater::any_sub() const
{
    return this->sub_window_flags.any();
}

bool RedrawingFlagsUpdater::any_stats() const
{
    return this->status_flags.any();
}

bool RedrawingFlagsUpdater::has(MainWindowRedrawingFlag flag) const
{
    return this->main_window_flags.has(flag);
}

bool RedrawingFlagsUpdater::has(SubWindowRedrawingFlag flag) const
{
    return this->sub_window_flags.has(flag);
}

bool RedrawingFlagsUpdater::has(StatusRecalculatingFlag flag) const
{
    return this->status_flags.has(flag);
}

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags) const
{
    return this->main_window_flags.has_any_of(flags);
}

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags) const
{
    return this->sub_window_flags.has_any_of(flags);
}

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<StatusRecalculatingFlag> &flags) const
{
    return this->status_flags.has_any_of(flags);
}

void RedrawingFlagsUpdater::set_flag(MainWindowRedrawingFlag flag)
{
    this->main_window_flags.set(flag);
}

void RedrawingFlagsUpdater::set_flag(SubWindowRedrawingFlag flag)
{
    this->sub_window_flags.set(flag);
}

void RedrawingFlagsUpdater::set_flag(StatusRecalculatingFlag flag)
{
    this->status_flags.set(flag);
}

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags)
{
    this->main_window_flags.set(flags);
}

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags)
{
    this->sub_window_flags.set(flags);
}

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<StatusRecalculatingFlag> &flags)
{
    this->status_flags.set(flags);
}

void RedrawingFlagsUpdater::reset_flag(MainWindowRedrawingFlag flag)
{
    this->main_window_flags.reset(flag);
}

void RedrawingFlagsUpdater::reset_flag(SubWindowRedrawingFlag flag)
{
    this->sub_window_flags.reset(flag);
}

void RedrawingFlagsUpdater::reset_flag(StatusRecalculatingFlag flag)
{
    this->status_flags.reset(flag);
}

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags)
{
    this->main_window_flags.reset(flags);
}

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags)
{
    this->sub_window_flags.reset(flags);
}

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<StatusRecalculatingFlag> &flags)
{
    this->status_flags.reset(flags);
}

void RedrawingFlagsUpdater::fill_up_sub_flags()
{
    constexpr auto all_sub_window_flags = EnumRange(SubWindowRedrawingFlag::INVENTORY, SubWindowRedrawingFlag::FOUND_ITEMS);
    this->sub_window_flags.set(all_sub_window_flags);
}

EnumClassFlagGroup<SubWindowRedrawingFlag> RedrawingFlagsUpdater::get_sub_intersection(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags)
{
    return this->sub_window_flags & flags;
}
