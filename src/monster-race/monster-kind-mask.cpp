#include "monster-race/monster-kind-mask.h"

const EnumClassFlagGroup<MonsterKindType> alignment_mask = {
    MonsterKindType::GOOD, MonsterKindType::EVIL
};

const EnumClassFlagGroup<MonsterKindType> has_corrupted_mind = {
    MonsterKindType::UNDEAD, MonsterKindType::DEMON
};
