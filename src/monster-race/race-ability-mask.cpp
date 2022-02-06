#include "monster-race/race-ability-mask.h"

// clang-format off
/* "summon" spells currently "summon" spells are included in "intelligent" and "indirect" */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_SUMMON_MASK = {
    MonsterAbilityType::S_KIN, MonsterAbilityType::S_CYBER, MonsterAbilityType::S_MONSTER, MonsterAbilityType::S_MONSTERS,
    MonsterAbilityType::S_ANT, MonsterAbilityType::S_SPIDER, MonsterAbilityType::S_HOUND, MonsterAbilityType::S_HYDRA,
    MonsterAbilityType::S_ANGEL, MonsterAbilityType::S_DEMON, MonsterAbilityType::S_UNDEAD, MonsterAbilityType::S_DRAGON,
    MonsterAbilityType::S_HI_UNDEAD, MonsterAbilityType::S_HI_DRAGON, MonsterAbilityType::S_AMBERITES, MonsterAbilityType::S_UNIQUE,
};

/* Choose "intelligent" spells when desperate Including "summon" spells */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_INT_MASK =
    EnumClassFlagGroup<MonsterAbilityType>(RF_ABILITY_SUMMON_MASK).set({
    MonsterAbilityType::DISPEL, MonsterAbilityType::HOLD, MonsterAbilityType::SLOW, MonsterAbilityType::CONF,
    MonsterAbilityType::BLIND, MonsterAbilityType::SCARE, MonsterAbilityType::BLINK, MonsterAbilityType::TPORT,
    MonsterAbilityType::TELE_LEVEL, MonsterAbilityType::TELE_AWAY, MonsterAbilityType::HEAL, MonsterAbilityType::INVULNER,
    MonsterAbilityType::HASTE, MonsterAbilityType::TRAPS,
});

/* Spells that cannot be used while player riding on the monster */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_RIDING_MASK = {
    MonsterAbilityType::SHRIEK, MonsterAbilityType::BLINK, MonsterAbilityType::TPORT, MonsterAbilityType::TRAPS,
    MonsterAbilityType::DARKNESS, MonsterAbilityType::SPECIAL,
};

/*
 * "bolt" spells that may hurt fellow monsters
 * Currently "bolt" spells are included in "attack"
 */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_BOLT_MASK = {
    MonsterAbilityType::ROCKET, MonsterAbilityType::SHOOT, MonsterAbilityType::BO_ACID, MonsterAbilityType::BO_ELEC,
    MonsterAbilityType::BO_FIRE, MonsterAbilityType::BO_COLD, MonsterAbilityType::BO_NETH, MonsterAbilityType::BO_WATE,
    MonsterAbilityType::BO_MANA, MonsterAbilityType::BO_PLAS, MonsterAbilityType::BO_ICEE, MonsterAbilityType::BO_VOID,
    MonsterAbilityType::BO_ABYSS, MonsterAbilityType::MISSILE,
};

/*
 * "beam" spells that may hurt fellow monsters
 * Currently "beam" spells are included in "attack"
 */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_BEAM_MASK = {
    MonsterAbilityType::PSY_SPEAR,
};

/*
 * "ball" spells with radius 4 that may hurt friends
 * Currently "radius 4 ball" spells are included in "ball"
 */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_BIG_BALL_MASK = {
    MonsterAbilityType::BA_CHAO, MonsterAbilityType::BA_LITE, MonsterAbilityType::BA_DARK, MonsterAbilityType::BA_WATE,
    MonsterAbilityType::BA_MANA, MonsterAbilityType::BA_VOID, MonsterAbilityType::BA_ABYSS,
};

/*
 * "breath" spells that may hurt friends
 * Currently "breath" spells are included in "ball" and "non-magic"
 */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_BREATH_MASK = {
    MonsterAbilityType::BR_ACID, MonsterAbilityType::BR_ELEC, MonsterAbilityType::BR_FIRE, MonsterAbilityType::BR_COLD,
    MonsterAbilityType::BR_POIS, MonsterAbilityType::BR_NETH, MonsterAbilityType::BR_LITE, MonsterAbilityType::BR_DARK,
    MonsterAbilityType::BR_CONF, MonsterAbilityType::BR_SOUN, MonsterAbilityType::BR_CHAO, MonsterAbilityType::BR_DISE,
    MonsterAbilityType::BR_NEXU, MonsterAbilityType::BR_SHAR, MonsterAbilityType::BR_TIME, MonsterAbilityType::BR_INER,
    MonsterAbilityType::BR_GRAV, MonsterAbilityType::BR_PLAS, MonsterAbilityType::BR_FORC, MonsterAbilityType::BR_MANA,
    MonsterAbilityType::BR_NUKE, MonsterAbilityType::BR_DISI, MonsterAbilityType::BR_VOID, MonsterAbilityType::BR_ABYSS,
};

/*
 * "ball" spells that may hurt friends
 * Including "radius 4 ball" and "breath" spells
 * Currently "ball" spells are included in "attack"
 */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_BALL_MASK =
    (RF_ABILITY_BIG_BALL_MASK | RF_ABILITY_BREATH_MASK).set({
    MonsterAbilityType::ROCKET, MonsterAbilityType::BA_NUKE, MonsterAbilityType::BA_ACID, MonsterAbilityType::BA_ELEC,
    MonsterAbilityType::BA_FIRE, MonsterAbilityType::BA_COLD, MonsterAbilityType::BA_POIS, MonsterAbilityType::BA_NETH,
});

/* "attack" spells including "bolt", "beam" and "ball" spells */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_ATTACK_MASK =
    (RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK | RF_ABILITY_BALL_MASK).set({
    MonsterAbilityType::DISPEL, MonsterAbilityType::DRAIN_MANA, MonsterAbilityType::MIND_BLAST, MonsterAbilityType::BRAIN_SMASH,
    MonsterAbilityType::CAUSE_1, MonsterAbilityType::CAUSE_2, MonsterAbilityType::CAUSE_3, MonsterAbilityType::CAUSE_4,
    MonsterAbilityType::SCARE, MonsterAbilityType::BLIND, MonsterAbilityType::CONF, MonsterAbilityType::SLOW, MonsterAbilityType::HOLD,
    MonsterAbilityType::HAND_DOOM, MonsterAbilityType::TELE_TO, MonsterAbilityType::TELE_AWAY, MonsterAbilityType::TELE_LEVEL,
    MonsterAbilityType::DARKNESS, MonsterAbilityType::TRAPS, MonsterAbilityType::FORGET,
});

/* "indirect" spells Including "summon" spells */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_INDIRECT_MASK =
    EnumClassFlagGroup<MonsterAbilityType>(RF_ABILITY_SUMMON_MASK).set({
    MonsterAbilityType::SHRIEK, MonsterAbilityType::HASTE, MonsterAbilityType::HEAL, MonsterAbilityType::INVULNER,
    MonsterAbilityType::BLINK, MonsterAbilityType::WORLD, MonsterAbilityType::TPORT, MonsterAbilityType::RAISE_DEAD,
});

/* "non-magic" spells including "breath" spells */
const EnumClassFlagGroup<MonsterAbilityType> RF_ABILITY_NOMAGIC_MASK =
    EnumClassFlagGroup<MonsterAbilityType>(RF_ABILITY_BREATH_MASK).set({
    MonsterAbilityType::SHRIEK, MonsterAbilityType::ROCKET, MonsterAbilityType::SHOOT, MonsterAbilityType::SPECIAL,
});
