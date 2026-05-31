#pragma once

#include <cstdint>
#include <string>
#include <tl/optional.hpp>
#include <vector>

enum class GameOptionPage : int;
enum class GameOptionType : int {
    ROGUE_LIKE_COMMANDS = 0,
    QUICK_MESSAGES = 1,
    OTHER_QUERY_FLAG = 2,
    CARRY_QUERY_FLAG = 3,
    USE_OLD_TARGET = 4,
    ALWAYS_PICKUP = 5,
    ALWAYS_REPEAT = 6,
    DEPTH_IN_FEET = 7,
    STACK_FORCE_NOTES = 8,
    STACK_FORCE_COSTS = 9,
    SHOW_LABELS = 10,
    SHOW_WEIGHTS = 11,
    // 12
    // 13
    RING_BELL = 14,
    // 15
    FIND_IGNORE_STAIRS = 16,
    FIND_IGNORE_DOORS = 17,
    FIND_CUT = 18,
    // 19
    DISTURB_MOVE = 20,
    DISTURB_NEAR = 21,
    DISTURB_PANEL = 22,
    DISTURB_STATE = 23,
    DISTURB_MINOR = 24,
    ALERT_TRAP_DETECT = 25,
    DISTURB_UNKNOWN = 26,
    DISTURB_TRAP_DETECT = 27,
    LAST_WORDS = 28,
    OVER_EXERT = 29,
    ALLOW_SMALLEST_FLOOR = 30,
    ALLOW_ARENA_FLOOR = 31,

    // 32
    VIEW_UNSAFE_WALLS = 33,
    VIEW_HIDDEN_WALLS = 34,
    DISTURB_HIGH = 35,
    // 36
    EXPAND_LIST = 37,
    VIEW_PERMA_GRIDS = 38,
    VIEW_TORCH_GRIDS = 39,
    VIEW_UNSAFE_GRIDS = 40,
    CONFIRM_QUEST = 41,
    FRESH_ONCE = 42,
    // 43
    EQUIPPY_CHARS = 44,
    // 45
    SMART_LEARN = 46,
    SMART_CHEAT = 47,
    // 48
    VIEW_REDUCE_VIEW = 49,
    CHECK_ABORT = 50,
    // 51
    FLUSH_FAILURE = 52,
    FLUSH_DISTURB = 53,
    // 54
    FRESH_BEFORE = 55,
    FRESH_AFTER = 56,
    FRESH_MESSAGE = 57,
    COMPRESS_SAVEFILE = 58,
    HILITE_PLAYER = 59,
    VIEW_YELLOW_LITE = 60,
    VIEW_BRIGHT_LITE = 61,
    VIEW_GRANITE_LITE = 62,
    VIEW_SPECIAL_LITE = 63,

    SHOW_ITEM_GRAPH = 64,
    BOUND_WALLS_PERM = 65,
    ALLOW_LARGEST_FLOOR = 66,
    ALWAYS_SMALL_FLOOR = 67,
    ALWAYS_LARGE_FLOOR = 68,
    TARGET_PET = 69,
    AUTO_MORE = 70,
    COMMAND_MENU = 71,
    DISPLAY_PATH = 72,
    // 73
    ABBREV_EXTRA = 74,
    ABBREV_ALL = 75,
    EXP_NEED = 76,
    IGNORE_UNVIEW = 77,
    SHOW_AMMO_DETAIL = 78,
    SHOW_AMMO_NO_CRIT = 79,
    SHOW_AMMO_CRIT_RATIO = 80,
    SHOW_ACTUAL_VALUE = 81,
    SKIP_MORE = 82,
    SHOW_LORE_SUMMARY = 83,
    // 84-94
    NUMPAD_AS_CURSORKEY = 95,

    // 96-127

    ALWAYS_SHOW_LIST = 128,
    // 129
    // 130
    POWERUP_HOME = 131,
    KEEP_SAVEFILE = 132,
    AUTO_DUMP = 133,
    SEND_SCORE = 134,
    // 135
    // 136
    // 137
    // 138
    RECORD_FIX_ART = 139,
    RECORD_RAND_ART = 140,
    RECORD_DESTROY_UNIQ = 141,
    RECORD_FIX_QUEST = 142,
    RECORD_RAND_QUEST = 143,
    RECORD_MAXDEPTH = 144,
    RECORD_STAIR = 145,
    RECORD_BUY = 146,
    RECORD_SELL = 147,
    RECORD_DANGER = 148,
    RECORD_ARENA = 149,
    RECORD_IDENT = 150,
    RECORD_NAMED_PET = 151,
    // 152-159

    DISPLAY_MUTATIONS = 160,
    PLAIN_DESCRIPTIONS = 161,
    // 162
    CONFIRM_DESTROY = 163,
    CONFIRM_WEAR = 164,
    // 165
    DISTURB_PETS = 166,
    EASY_OPEN = 167,
    EASY_DISARM = 168,
    EASY_FLOOR = 169,
    USE_COMMAND = 170,
    CENTER_PLAYER = 171,
    CENTER_RUNNING = 172,
    // 173-191

    VANILLA_TOWN = 192,
    LITE_TOWN = 193,
    IRONMAN_SHOPS = 194,
    IRONMAN_SMALLEST_FLOOR = 195,
    IRONMAN_DOWNWARD = 196,
    // 197
    PLAIN_PICKUP = 198,
    // 199
    IRONMAN_FORCE_ARENA_FLOOR = 200,
    // 201
    // 202
    ALLOW_DEBUG_OPTS = 203,
    IRONMAN_ROOMS = 204,
    LEFT_HANDER = 205,
    PRESERVE_MODE = 206,
    AUTOROLLER = 207,
    AUTOCHARA = 208,
    // 209
    IRONMAN_NIGHTMARE = 210,
    // 211-223

    DESTROY_ITEMS = 224,
    LEAVE_SPECIAL = 225,
    LEAVE_WORTH = 226,
    LEAVE_EQUIP = 227,
    LEAVE_WANTED = 228,
    LEAVE_CORPSE = 229,
    LEAVE_JUNK = 230,
    LEAVE_CHEST = 231,
    DESTROY_FEELING = 232,
    DESTROY_IDENTIFY = 233,
    // 234-255

    MAX = 256,
};
class GameOption {
public:
    GameOption(bool *value, bool norm, GameOptionType type, std::string &&text, std::string &&description, const tl::optional<GameOptionPage> &page = tl::nullopt);
    GameOption(bool *value, bool norm, uint8_t set, uint8_t bits, std::string &&text, std::string &&description, const tl::optional<GameOptionPage> &page = tl::nullopt);
    bool *value;
    bool default_value;
    uint8_t flag_position;
    uint8_t offset;
    std::string text;
    std::string description;
    tl::optional<GameOptionPage> page;
};

extern const std::vector<GameOption> option_info;
extern const std::vector<GameOption> cheat_info;
extern const std::vector<GameOption> autosave_info;
