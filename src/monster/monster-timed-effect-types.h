#pragma once

enum monster_timed_effect_type {
    MTIMED_CSLEEP = 0, /* Monster is sleeping */
    MTIMED_FAST = 1, /* Monster is temporarily fast */
    MTIMED_SLOW = 2, /* Monster is temporarily slow */
    MTIMED_STUNNED = 3, /* Monster is stunned */
    MTIMED_CONFUSED = 4, /* Monster is confused */
    MTIMED_MONFEAR = 5, /* Monster is afraid */
    MTIMED_INVULNER = 6, /* Monster is temporarily invulnerable */
    MAX_MTIMED = 7,
};
