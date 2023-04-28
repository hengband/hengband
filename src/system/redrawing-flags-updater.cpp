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

bool RedrawingFlagsUpdater::has(StatusRedrawingFlag flag) const
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

bool RedrawingFlagsUpdater::has_any_of(const EnumClassFlagGroup<StatusRedrawingFlag> &flags) const
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

void RedrawingFlagsUpdater::set_flag(StatusRedrawingFlag flag)
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

void RedrawingFlagsUpdater::set_flags(const EnumClassFlagGroup<StatusRedrawingFlag> &flags)
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

void RedrawingFlagsUpdater::reset_flag(StatusRedrawingFlag flag)
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

void RedrawingFlagsUpdater::reset_flags(const EnumClassFlagGroup<StatusRedrawingFlag> &flags)
{
    this->status_flags.reset(flags);
}

void RedrawingFlagsUpdater::fill_up_sub_flags()
{
    for (const auto flag : EnumRange(SubWindowRedrawingFlag::INVENTORY, SubWindowRedrawingFlag::FOUND_ITEMS)) {
        this->sub_window_flags.set(flag);
    }
}
