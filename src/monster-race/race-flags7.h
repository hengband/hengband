#pragma once

enum race_flags7 {
    RF7_AQUATIC = 0x00000001, /* Aquatic monster */
    RF7_CAN_SWIM = 0x00000002, /* Monster can swim */
    RF7_CAN_FLY = 0x00000004, /* Monster can fly */
    RF7_FRIENDLY = 0x00000008, /* Monster is friendly */
    RF7_NAZGUL = 0x00000010, /* Is a "Nazgul" unique */
    RF7_UNIQUE2 = 0x00000020, /* Fake unique */
    RF7_RIDING = 0x00000040, /* Good for riding */
    RF7_KAGE = 0x00000080, /* Is kage */
    RF7_HAS_LITE_1 = 0x00000100, /* Monster carries light */
    RF7_SELF_LITE_1 = 0x00000200, /* Monster lights itself */
    RF7_HAS_LITE_2 = 0x00000400, /* Monster carries light */
    RF7_SELF_LITE_2 = 0x00000800, /* Monster lights itself */
    RF7_GUARDIAN = 0x00001000, /* Guardian of a dungeon */
    RF7_CHAMELEON = 0x00002000, /* Chameleon can change */
    RF7_XXXX4XXX = 0x00004000, /* Now Empty */
    RF7_TANUKI = 0x00008000, /* Tanuki disguise */
    RF7_HAS_DARK_1 = 0x00010000, /* Monster carries darkness */
    RF7_SELF_DARK_1 = 0x00020000, /* Monster darkens itself */
    RF7_HAS_DARK_2 = 0x00040000, /* Monster carries darkness */
    RF7_SELF_DARK_2 = 0x00080000, /* Monster darkens itself */
};

#define RF7_LITE_MASK (RF7_HAS_LITE_1 | RF7_SELF_LITE_1 | RF7_HAS_LITE_2 | RF7_SELF_LITE_2)
#define RF7_DARK_MASK (RF7_HAS_DARK_1 | RF7_SELF_DARK_1 | RF7_HAS_DARK_2 | RF7_SELF_DARK_2)
#define RF7_HAS_LD_MASK (RF7_HAS_LITE_1 | RF7_HAS_LITE_2 | RF7_HAS_DARK_1 | RF7_HAS_DARK_2)
#define RF7_SELF_LD_MASK (RF7_SELF_LITE_1 | RF7_SELF_LITE_2 | RF7_SELF_DARK_1 | RF7_SELF_DARK_2)
