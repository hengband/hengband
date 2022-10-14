#include "grid/feature-action-flags.h"

/*!
 * @brief 地形状態フラグテーブル /
 * The table of features' actions
 */
const byte terrain_action_flags[FF_FLAG_MAX] = {
    0, /* LOS */
    0, /* PROJECT */
    0, /* MOVE */
    0, /* PLACE */
    0, /* DROP */
    0, /* SECRET */
    0, /* NOTICE */
    0, /* REMEMBER */
    0, /* OPEN */
    0, /* CLOSE */
    FAF_CRASH_GLASS, /* BASH */
    0, /* SPIKE */
    FAF_DESTROY, /* DISARM */
    0, /* STORE */
    FAF_DESTROY | FAF_CRASH_GLASS, /* TUNNEL */
    0, /* MAY_HAVE_GOLD */
    0, /* HAS_GOLD */
    0, /* HAS_ITEM */
    0, /* DOOR */
    0, /* TRAP */
    0, /* STAIRS */
    0, /* RUNE_OF_PROTECTION */
    0, /* LESS */
    0, /* MORE */
    0, /* RUN */
    0, /* FLOOR */
    0, /* WALL */
    0, /* PERMANENT */
    0, /* INNER */
    0, /* OUTER */
    0, /* SOLID */
    0, /* HIT_TRAP */

    0, /* BRIDGE */
    0, /* RIVER */
    0, /* LAKE */
    0, /* BRIDGED */
    0, /* COVERED */
    0, /* GLOW */
    0, /* ENSECRET */
    0, /* WATER */
    0, /* LAVA */
    0, /* SHALLOW */
    0, /* DEEP */
    0, /* FILLED */
    FAF_DESTROY | FAF_CRASH_GLASS, /* HURT_ROCK */
    0, /* HURT_FIRE */
    0, /* HURT_COLD */
    0, /* HURT_ACID */
    0, /* ICE */
    0, /* ACID */
    0, /* OIL */
    0, /* XXX04 */
    0, /* CAN_CLIMB */
    0, /* CAN_FLY */
    0, /* CAN_SWIM */
    0, /* CAN_PASS */
    0, /* CAN_OOZE */
    0, /* CAN_DIG */
    0, /* HIDE_ITEM */
    0, /* HIDE_SNEAK */
    0, /* HIDE_SWIM */
    0, /* HIDE_DIG */
    0, /* KILL_HUGE */
    0, /* KILL_MOVE */

    0, /* PICK_TRAP */
    0, /* PICK_DOOR */
    0, /* ALLOC */
    0, /* CHEST */
    0, /* DROP_1D2 */
    0, /* DROP_2D2 */
    0, /* DROP_GOOD */
    0, /* DROP_GREAT */
    0, /* HURT_POIS */
    0, /* HURT_ELEC */
    0, /* HURT_WATER */
    0, /* HURT_BWATER */
    0, /* USE_FEAT */
    0, /* GET_FEAT */
    0, /* GROUND */
    0, /* OUTSIDE */
    0, /* EASY_HIDE */
    0, /* EASY_CLIMB */
    0, /* MUST_CLIMB */
    0, /* TREE */
    0, /* NEED_TREE */
    0, /* BLOOD */
    0, /* DUST */
    0, /* SLIME */
    0, /* PLANT */
    0, /* XXX2 */
    0, /* INSTANT */
    0, /* EXPLODE */
    0, /* TIMED */
    0, /* ERUPT */
    0, /* STRIKE */
    0, /* SPREAD */

    0, /* SPECIAL */
    FAF_DESTROY | FAF_NO_DROP | FAF_CRASH_GLASS, /* HURT_DISI */
    0, /* QUEST_ENTER */
    0, /* QUEST_EXIT */
    0, /* QUEST */
    0, /* SHAFT */
    0, /* MOUNTAIN */
    0, /* BLDG */
    0, /* RUNE_OF_EXPLOSION */
    0, /* PATTERN */
    0, /* TOWN */
    0, /* ENTRANCE */
    0, /* MIRROR */
    0, /* UNPERM */
    0, /* TELEPORTABLE */
    0, /* CONVERT */
    0, /* GLASS */
};
