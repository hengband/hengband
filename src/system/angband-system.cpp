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

uint32_t AngbandSystem::get_seed_flavor() const
{
    return this->seed_flavor;
}

void AngbandSystem::set_seed_flavor(const uint32_t seed)
{
    this->seed_flavor = seed;
}

uint32_t AngbandSystem::get_seed_town() const
{
    return this->seed_town;
}

void AngbandSystem::set_seed_town(const uint32_t seed)
{
    this->seed_town = seed;
}

Xoshiro128StarStar &AngbandSystem::get_rng()
{
    return this->rng;
}

void AngbandSystem::set_rng(const Xoshiro128StarStar &rng_)
{
    this->rng = rng_;
}
