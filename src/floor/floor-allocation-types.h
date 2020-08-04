#pragma once

/* Dungeon allocation "places" */
typedef enum dap_type {
	ALLOC_SET_CORR = 1, /* Hallway */
	ALLOC_SET_ROOM = 2, /* Room */
	ALLOC_SET_BOTH = 3, /* Anywhere */
} dap_type;

/* Dungeon allocation "types" */
typedef enum dungeon_allocation_type {
	ALLOC_TYP_RUBBLE = 1, /* Rubble */
    ALLOC_TYP_TRAP = 3, /* Trap */
    ALLOC_TYP_GOLD = 4, /* Gold */
    ALLOC_TYP_OBJECT = 5, /* Object */
    ALLOC_TYP_INVIS = 6, /* Invisible wall */
} dungeon_allocation_type;
