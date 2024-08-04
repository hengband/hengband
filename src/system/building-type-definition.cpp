#include "system/building-type-definition.h"

std::array<building_type, MAX_BUILDINGS> buildings;
int battle_odds;
int wager_melee;
int bet_number;

MeleeGladiator::MeleeGladiator(MonsterRaceId monrace_id, uint32_t odds)
    : monrace_id(monrace_id)
    , odds(odds)
{
}

MeleeArena MeleeArena::instance{};

MeleeArena &MeleeArena::get_instance()
{
    return instance;
}

MeleeGladiator &MeleeArena::get_gladiator(int n)
{
    return this->gladiators.at(n);
}

const MeleeGladiator &MeleeArena::get_gladiator(int n) const
{
    return this->gladiators.at(n);
}

void MeleeArena::set_gladiator(int n, const MeleeGladiator &gladiator)
{
    this->gladiators[n] = gladiator;
}

const std::array<MeleeGladiator, NUM_GLADIATORS> &MeleeArena::get_gladiators() const
{
    return this->gladiators;
}

std::array<MeleeGladiator, NUM_GLADIATORS> &MeleeArena::get_gladiators()
{
    return this->gladiators;
}
