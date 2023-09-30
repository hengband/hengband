#include "system/angband-system.h"

AngbandSystem AngbandSystem::instance{};

AngbandSystem &AngbandSystem::get_instance()
{
    return instance;
}
