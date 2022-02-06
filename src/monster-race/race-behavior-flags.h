#pragma once

enum class MonsterBehaviorType {
    NEVER_BLOW = 0,
    NEVER_MOVE = 1,
    OPEN_DOOR = 2,
    BASH_DOOR = 3,
    MOVE_BODY = 4,
    KILL_BODY = 5,
    TAKE_ITEM = 6,
    KILL_ITEM = 7,
    RAND_MOVE_25 = 8,
    RAND_MOVE_50 = 9,
    STUPID = 10,
    SMART = 11,
    FRIENDLY = 12,
    PREVENT_SUDDEN_MAGIC = 13,
    MAX,
};
