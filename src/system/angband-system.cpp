#include "system/angband-system.h"
#include "term/z-rand.h" // @todo 相互参照、後で考える.

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

void AngbandSystem::initialize_seed_flavor()
{
    this->seed_flavor = randint0(0x10000000);
}

void AngbandSystem::initialize_seed_town()
{
    this->seed_town = randint0(0x10000000);
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

std::string AngbandSystem::get_seed_town(std::string_view param) const
{
    return std::to_string(this->seed_town % std::stoi(param.data()));
}

void AngbandSystem::set_seed_town(const uint32_t seed)
{
    this->seed_town = seed;
}

Xoshiro128StarStar &AngbandSystem::get_rng()
{
    return this->rng;
}

const Xoshiro128StarStar::state_type &AngbandSystem::get_rng_state() const
{
    return this->rng.get_state();
}

void AngbandSystem::set_rng_state(const Xoshiro128StarStar::state_type &state)
{
    this->rng.set_state(state);
}

Xoshiro128StarStar AngbandSystem::initialize_rng()
{
    return this->initialize_rng(this->seed_flavor);
}

Xoshiro128StarStar AngbandSystem::initialize_rng(const uint32_t seed)
{
    const auto old = this->rng;
    this->rng.set_state(seed);
    return old;
}

void AngbandSystem::set_rng(const Xoshiro128StarStar &rng_)
{
    this->rng = rng_;
}
