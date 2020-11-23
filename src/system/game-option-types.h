#pragma once

typedef enum game_option_types {
	OPT_PAGE_INPUT = 1,
    OPT_PAGE_MAPSCREEN = 2,
    OPT_PAGE_TEXT = 3,
    OPT_PAGE_GAMEPLAY = 4,
    OPT_PAGE_DISTURBANCE = 5,
    OPT_PAGE_BIRTH = 6,
    OPT_PAGE_AUTODESTROY = 7,
    OPT_PAGE_PLAYRECORD = 10,
} game_option_types;

#define OPT_PAGE_JAPANESE_ONLY 99
