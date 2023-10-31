#include "system/angband-system.h"

AngbandSystem AngbandSystem::instance{};

AngbandSystem &AngbandSystem::get_instance()
{
    return instance;
}

void AngbandSystem::set_phase_out(bool new_status)
{
    this->phase_out_stat = new_status;
}

bool AngbandSystem::is_phase_out() const
{
    return this->phase_out_stat;
}

/*!
 * @brief プレイヤーの攻撃射程
 * @return 射程
 */
int AngbandSystem::get_max_range() const
{
    return this->phase_out_stat ? 36 : 18;
}
