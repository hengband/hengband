#pragma once

enum class MonsterWildernessType {
    WILD_ONLY = 0,
    WILD_TOWN = 1,
    WILD_SHORE = 2,
    WILD_OCEAN = 3,
    WILD_WASTE = 4,
    WILD_WOOD = 5,
    WILD_VOLCANO = 6,
    WILD_MOUNTAIN = 7,
    WILD_GRASS = 8,
    WILD_SWAMP = 9, //!< 沼地に生息(未使用)
    WILD_ALL = 10,
    MAX,
};
