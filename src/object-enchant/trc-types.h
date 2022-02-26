#pragma once

/* 旧TRC. 実態に即した名称に変更 */
enum class CurseTraitType {
    CURSED = 0,
    HEAVY_CURSE = 1,
    PERMA_CURSE = 2,
    XXX1 = 3,
    TY_CURSE = 4,
    AGGRAVATE = 5,
    DRAIN_EXP = 6,
    SLOW_REGEN = 7,
    ADD_L_CURSE = 8,
    ADD_H_CURSE = 9,
    CALL_ANIMAL = 10,
    CALL_DEMON = 11,
    CALL_DRAGON = 12,
    COWARDICE = 13,
    TELEPORT = 14,
    LOW_MELEE = 15,
    LOW_AC = 16,
    HARD_SPELL = 17,
    FAST_DIGEST = 18,
    DRAIN_HP = 19,
    DRAIN_MANA = 20,
    CALL_UNDEAD = 21,
    BERS_RAGE = 22,
    PERSISTENT_CURSE = 23,
    VUL_CURSE = 24,
    MAX,
};

enum class CurseSpecialTraitType {
    TELEPORT_SELF = 0,
    CHAINSWORD = 1,
    MAX,
};
