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

/*!
 * @brief プレイヤーの攻撃射程
 * @return 射程
 */
int AngbandSystem::get_max_range() const
{
    return this->watch_stat ? 36 : 18;
}
