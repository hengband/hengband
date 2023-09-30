#include "system/angband-system.h"

AngbandSystem AngbandSystem::instance{};

AngbandSystem &AngbandSystem::get_instance()
{
    return instance;
}

void AngbandSystem::set_watch(bool new_status)
{
    this->watch_stat = new_status;
}

bool AngbandSystem::is_watching() const
{
    return this->watch_stat;
}
