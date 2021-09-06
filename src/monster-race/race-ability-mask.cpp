#include "monster-race/race-ability-mask.h"

// clang-format off
/* "summon" spells currently "summon" spells are included in "intelligent" and "indirect" */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_SUMMON_MASK = {
    RF_ABILITY::S_KIN, RF_ABILITY::S_CYBER, RF_ABILITY::S_MONSTER, RF_ABILITY::S_MONSTERS,
    RF_ABILITY::S_ANT, RF_ABILITY::S_SPIDER, RF_ABILITY::S_HOUND, RF_ABILITY::S_HYDRA,
    RF_ABILITY::S_ANGEL, RF_ABILITY::S_DEMON, RF_ABILITY::S_UNDEAD, RF_ABILITY::S_DRAGON,
    RF_ABILITY::S_HI_UNDEAD, RF_ABILITY::S_HI_DRAGON, RF_ABILITY::S_AMBERITES, RF_ABILITY::S_UNIQUE,
};

/* Choose "intelligent" spells when desperate Including "summon" spells */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_INT_MASK =
    EnumClassFlagGroup<RF_ABILITY>(RF_ABILITY_SUMMON_MASK).set({
    RF_ABILITY::DISPEL, RF_ABILITY::HOLD, RF_ABILITY::SLOW, RF_ABILITY::CONF,
    RF_ABILITY::BLIND, RF_ABILITY::SCARE, RF_ABILITY::BLINK, RF_ABILITY::TPORT,
    RF_ABILITY::TELE_LEVEL, RF_ABILITY::TELE_AWAY, RF_ABILITY::HEAL, RF_ABILITY::INVULNER,
    RF_ABILITY::HASTE, RF_ABILITY::TRAPS,
});

/* Spells that cannot be used while player riding on the monster */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_RIDING_MASK = {
    RF_ABILITY::SHRIEK, RF_ABILITY::BLINK, RF_ABILITY::TPORT, RF_ABILITY::TRAPS,
    RF_ABILITY::DARKNESS, RF_ABILITY::SPECIAL,
};

/*
 * "bolt" spells that may hurt fellow monsters
 * Currently "bolt" spells are included in "attack"
 */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_BOLT_MASK = {
    RF_ABILITY::ROCKET, RF_ABILITY::SHOOT, RF_ABILITY::BO_ACID, RF_ABILITY::BO_ELEC,
    RF_ABILITY::BO_FIRE, RF_ABILITY::BO_COLD, RF_ABILITY::BO_NETH, RF_ABILITY::BO_WATE,
    RF_ABILITY::BO_MANA, RF_ABILITY::BO_PLAS, RF_ABILITY::BO_ICEE, RF_ABILITY::MISSILE,
};

/*
 * "beam" spells that may hurt fellow monsters
 * Currently "beam" spells are included in "attack"
 */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_BEAM_MASK = {
    RF_ABILITY::PSY_SPEAR,
};

/*
 * "ball" spells with radius 4 that may hurt friends
 * Currently "radius 4 ball" spells are included in "ball"
 */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_BIG_BALL_MASK = {
    RF_ABILITY::BA_CHAO, RF_ABILITY::BA_LITE, RF_ABILITY::BA_DARK, RF_ABILITY::BA_WATE,
    RF_ABILITY::BA_MANA,
};

/*
 * "breath" spells that may hurt friends
 * Currently "breath" spells are included in "ball" and "non-magic"
 */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_BREATH_MASK = {
    RF_ABILITY::BR_ACID, RF_ABILITY::BR_ELEC, RF_ABILITY::BR_FIRE, RF_ABILITY::BR_COLD,
    RF_ABILITY::BR_POIS, RF_ABILITY::BR_NETH, RF_ABILITY::BR_LITE, RF_ABILITY::BR_DARK,
    RF_ABILITY::BR_CONF, RF_ABILITY::BR_SOUN, RF_ABILITY::BR_CHAO, RF_ABILITY::BR_DISE,
    RF_ABILITY::BR_NEXU, RF_ABILITY::BR_SHAR, RF_ABILITY::BR_TIME, RF_ABILITY::BR_INER,
    RF_ABILITY::BR_GRAV, RF_ABILITY::BR_PLAS, RF_ABILITY::BR_FORC, RF_ABILITY::BR_MANA,
    RF_ABILITY::BR_NUKE, RF_ABILITY::BR_DISI,
};

/*
 * "ball" spells that may hurt friends
 * Including "radius 4 ball" and "breath" spells
 * Currently "ball" spells are included in "attack"
 */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_BALL_MASK =
    (RF_ABILITY_BIG_BALL_MASK | RF_ABILITY_BREATH_MASK).set({
    RF_ABILITY::ROCKET, RF_ABILITY::BA_NUKE, RF_ABILITY::BA_ACID, RF_ABILITY::BA_ELEC,
    RF_ABILITY::BA_FIRE, RF_ABILITY::BA_COLD, RF_ABILITY::BA_POIS, RF_ABILITY::BA_NETH,
});

/* "attack" spells including "bolt", "beam" and "ball" spells */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_ATTACK_MASK =
    (RF_ABILITY_BOLT_MASK | RF_ABILITY_BEAM_MASK | RF_ABILITY_BALL_MASK).set({
    RF_ABILITY::DISPEL, RF_ABILITY::DRAIN_MANA, RF_ABILITY::MIND_BLAST, RF_ABILITY::BRAIN_SMASH,
    RF_ABILITY::CAUSE_1, RF_ABILITY::CAUSE_2, RF_ABILITY::CAUSE_3, RF_ABILITY::CAUSE_4,
    RF_ABILITY::SCARE, RF_ABILITY::BLIND, RF_ABILITY::CONF, RF_ABILITY::SLOW, RF_ABILITY::HOLD,
    RF_ABILITY::HAND_DOOM, RF_ABILITY::TELE_TO, RF_ABILITY::TELE_AWAY, RF_ABILITY::TELE_LEVEL,
    RF_ABILITY::DARKNESS, RF_ABILITY::TRAPS, RF_ABILITY::FORGET,
});

/* "indirect" spells Including "summon" spells */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_INDIRECT_MASK =
    EnumClassFlagGroup<RF_ABILITY>(RF_ABILITY_SUMMON_MASK).set({
    RF_ABILITY::SHRIEK, RF_ABILITY::HASTE, RF_ABILITY::HEAL, RF_ABILITY::INVULNER,
    RF_ABILITY::BLINK, RF_ABILITY::WORLD, RF_ABILITY::TPORT, RF_ABILITY::RAISE_DEAD,
});

/* "non-magic" spells including "breath" spells */
const EnumClassFlagGroup<RF_ABILITY> RF_ABILITY_NOMAGIC_MASK =
    EnumClassFlagGroup<RF_ABILITY>(RF_ABILITY_BREATH_MASK).set({
    RF_ABILITY::SHRIEK, RF_ABILITY::ROCKET, RF_ABILITY::SHOOT, RF_ABILITY::SPECIAL,
});
