#include "system/angband-system.h"
#include "util/string-processor.h"

AngbandSystem AngbandSystem::instance{};

std::string AngbandVersion::build_expression(bool includes_extra) const
{
    if (includes_extra) {
        return format("%d.%d.%d.%d", this->major, this->minor, this->patch, this->extra);
    }

    return format("%d.%d.%d", this->major, this->minor, this->patch);
}

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

AngbandVersion &AngbandSystem::get_version()
{
    return this->version;
}

const AngbandVersion &AngbandSystem::get_version() const
{
    return this->version;
}

void AngbandSystem::set_version(const AngbandVersion &new_version)
{
    this->version = new_version;
}

std::string AngbandSystem::build_version_expression(bool includes_extra) const
{
    return this->version.build_expression(includes_extra);
}
