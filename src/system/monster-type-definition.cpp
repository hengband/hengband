#include "system/monster-type-definition.h"

bool monster_type::is_friendly() const
{
    return this->mflag2.has(MonsterConstantFlagType::FRIENDLY);
}

bool monster_type::is_pet() const
{
    return this->mflag2.has(MonsterConstantFlagType::PET);
}

bool monster_type::is_hostile() const
{
    return !this->is_friendly() && !this->is_pet();
}
