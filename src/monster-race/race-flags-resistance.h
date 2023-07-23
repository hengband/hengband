#pragma once

enum class MonsterResistanceType {
    RESIST_ALL = 0, /* Resist all */
    HURT_ACID = 1, /* Hurt badly by acid */
    RESIST_ACID = 2, /* Resist acid */
    IMMUNE_ACID = 3, /* Immunity acid */
    HURT_ELEC = 4, /* Hurt badly by elec */
    RESIST_ELEC = 5, /* Resist elec */
    IMMUNE_ELEC = 6, /* Immunity elec */
    HURT_FIRE = 7, /* Hurt badly by fire */
    RESIST_FIRE = 8, /* Resist fire */
    IMMUNE_FIRE = 9, /* Immunity fire */
    HURT_COLD = 10, /* Hurt badly by cold */
    RESIST_COLD = 11, /* Resist cold */
    IMMUNE_COLD = 12, /* Immunity cold */
    HURT_POISON = 13, /* Hurt badly by poison */
    RESIST_POISON = 14, /* Resist poison */
    IMMUNE_POISON = 15, /* Immunity poison */
    HURT_LITE = 16, /* Hurt badly by lite */
    RESIST_LITE = 17, /* Resist lite */
    HURT_DARK = 18, /* Hurt badly by dark */
    RESIST_DARK = 19, /* Resist dark */
    HURT_NETHER = 20, /* Hurt badly by nether */
    RESIST_NETHER = 21, /* Resist nether */
    HURT_WATER = 22, /* Hurt badly by water */
    RESIST_WATER = 23, /* Resist water */
    HURT_PLASMA = 24, /* Hurt badly by plasma */
    RESIST_PLASMA = 25, /* Resist plasma */
    HURT_SHARDS = 26, /* Hurt badly by shards */
    RESIST_SHARDS = 27, /* Resist shards */
    HURT_SOUND = 28, /* Hurt badly by sound */
    RESIST_SOUND = 29, /* Resist sound */
    HURT_CHAOS = 30, /* Hurt badly by chaos */
    RESIST_CHAOS = 31, /* Resist chaos */
    HURT_NEXUS = 32, /* Hurt badly by nexus */
    RESIST_NEXUS = 33, /* Resist nexus */
    HURT_DISENCHANT = 34, /* Hurt badly by disenchantment */
    RESIST_DISENCHANT = 35, /* Resist disenchantment */
    HURT_FORCE = 36, /* Hurt badly by force */
    RESIST_FORCE = 37, /* Resist force */
    HURT_INERTIA = 38, /* Hurt badly by inertia */
    RESIST_INERTIA = 39, /* Resist inertia */
    HURT_TIME = 40, /* Hurt badly by time */
    RESIST_TIME = 41, /* Resist time */
    HURT_GRAVITY = 42, /* Hurt badly by gravity */
    RESIST_GRAVITY = 43, /* Resist gravity */
    RESIST_TELEPORT = 44, /* Resist teleportation */
    HURT_ROCK = 45, /* Hurt by rock remover */
    RESIST_ROCK = 46, /* Resist rock remover */
    HURT_ABYSS = 47, /* Hurt badly by abyss */
    RESIST_ABYSS = 48, /* Resist abyss */
    HURT_VOID_MAGIC = 49, /* Hurt badly by void */
    RESIST_VOID_MAGIC = 50, /* Resist void */
    MAX, /* Max of Resistances */
};
