#pragma once

enum class MonsterBrightnessType {
    HAS_LITE_1 = 0, /* Monster carries light */
    SELF_LITE_1 = 1, /* Monster lights itself */
    HAS_DARK_1 = 2, /* Monster carries darkness */
    SELF_DARK_1 = 3, /* Monster darkens itself */
    HAS_LITE_2 = 4, /* Monster carries light */
    SELF_LITE_2 = 5, /* Monster lights itself */
    HAS_DARK_2 = 6, /* Monster carries darkness */
    SELF_DARK_2 = 7, /* Monster darkens itself */
    MAX,
};
