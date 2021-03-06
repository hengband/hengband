#pragma once

enum race_flags_ability2 {
	RF6_HASTE = 0x00000001, /* Speed self */
    RF6_HAND_DOOM = 0x00000002, /* Hand of Doom */
    RF6_HEAL = 0x00000004, /* Heal self */
    RF6_INVULNER = 0x00000008, /* INVULNERABILITY! */
    RF6_BLINK = 0x00000010, /* Teleport Short */
    RF6_TPORT = 0x00000020, /* Teleport Long */
    RF6_WORLD = 0x00000040, /* world */
    RF6_SPECIAL = 0x00000080, /* Special Attack */
    RF6_TELE_TO = 0x00000100, /* Move player to monster */
    RF6_TELE_AWAY = 0x00000200, /* Move player far away */
    RF6_TELE_LEVEL = 0x00000400, /* Move player vertically */
    RF6_PSY_SPEAR = 0x00000800, /* Psyco-spear */
    RF6_DARKNESS = 0x00001000, /* Create Darkness */
    RF6_TRAPS = 0x00002000, /* Create Traps */
    RF6_FORGET = 0x00004000, /* Cause amnesia */
    RF6_RAISE_DEAD = 0x00008000, /* Raise Dead */
    RF6_S_KIN = 0x00010000, /* Summon "kin" */
    RF6_S_CYBER = 0x00020000, /* Summon Cyberdemons! */
    RF6_S_MONSTER = 0x00040000, /* Summon Monster */
    RF6_S_MONSTERS = 0x00080000, /* Summon Monsters */
    RF6_S_ANT = 0x00100000, /* Summon Ants */
    RF6_S_SPIDER = 0x00200000, /* Summon Spiders */
    RF6_S_HOUND = 0x00400000, /* Summon Hounds */
    RF6_S_HYDRA = 0x00800000, /* Summon Hydras */
    RF6_S_ANGEL = 0x01000000, /* Summon Angel */
    RF6_S_DEMON = 0x02000000, /* Summon Demon */
    RF6_S_UNDEAD = 0x04000000, /* Summon Undead */
    RF6_S_DRAGON = 0x08000000, /* Summon Dragon */
    RF6_S_HI_UNDEAD = 0x10000000, /* Summon Greater Undead */
    RF6_S_HI_DRAGON = 0x20000000, /* Summon Ancient Dragon */
    RF6_S_AMBERITES = 0x40000000, /* Summon Amberites */
    RF6_S_UNIQUE = 0x80000000, /* Summon Unique Monster */
};
