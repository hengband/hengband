#include "monster-race/race-resistance-mask.h"

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_IM_ACID_MASK = {
    MonsterResistanceType::IMMUNE_ACID,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_IM_ELEC_MASK = {
    MonsterResistanceType::IMMUNE_ELEC,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_IM_FIRE_MASK = {
    MonsterResistanceType::IMMUNE_FIRE,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_IM_COLD_MASK = {
    MonsterResistanceType::IMMUNE_COLD,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_IM_POISON_MASK = {
    MonsterResistanceType::IMMUNE_POISON,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_ACID_MASK = EnumClassFlagGroup<MonsterResistanceType>(RFR_EFF_IM_ACID_MASK).set(MonsterResistanceType::RESIST_ACID);

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_ELEC_MASK = EnumClassFlagGroup<MonsterResistanceType>(RFR_EFF_IM_ELEC_MASK).set(MonsterResistanceType::RESIST_ELEC);

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_FIRE_MASK = EnumClassFlagGroup<MonsterResistanceType>(RFR_EFF_IM_FIRE_MASK).set(MonsterResistanceType::RESIST_FIRE);

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_COLD_MASK = EnumClassFlagGroup<MonsterResistanceType>(RFR_EFF_IM_COLD_MASK).set(MonsterResistanceType::RESIST_COLD);

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_POISON_MASK = EnumClassFlagGroup<MonsterResistanceType>(RFR_EFF_IM_POISON_MASK).set(MonsterResistanceType::RESIST_POISON);

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_SHARDS_MASK = {
    MonsterResistanceType::RESIST_SHARDS,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_CHAOS_MASK = {
    MonsterResistanceType::RESIST_CHAOS,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_RESIST_NEXUS_MASK = {
    MonsterResistanceType::RESIST_NEXUS,
    MonsterResistanceType::RESIST_ALL,
};

const EnumClassFlagGroup<MonsterResistanceType> RFR_EFF_IMMUNE_ELEMENT_MASK = {
    MonsterResistanceType::IMMUNE_ACID,
    MonsterResistanceType::IMMUNE_ELEC,
    MonsterResistanceType::IMMUNE_FIRE,
    MonsterResistanceType::IMMUNE_COLD,
    MonsterResistanceType::IMMUNE_POISON,
};