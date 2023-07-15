#pragma once

#include "system/angband.h"

#define NUM_NEST_MON_TYPE 64 /*!<nestの種別数 */
#define TRAPPED_PIT_MONSTER_PLACE_MAX 69

enum class MonsterRaceId : int16_t;

/*! nestのID定義 /  Nest types code */
enum nest_type : int {
    NEST_TYPE_CLONE = 0,
    NEST_TYPE_JELLY = 1,
    NEST_TYPE_SYMBOL_GOOD = 2,
    NEST_TYPE_SYMBOL_EVIL = 3,
    NEST_TYPE_MIMIC = 4,
    NEST_TYPE_LOVECRAFTIAN = 5,
    NEST_TYPE_KENNEL = 6,
    NEST_TYPE_ANIMAL = 7,
    NEST_TYPE_CHAPEL = 8,
    NEST_TYPE_UNDEAD = 9,
};

/*! pitのID定義 / Pit types code */
enum pit_type : int {
    PIT_TYPE_ORC = 0,
    PIT_TYPE_TROLL = 1,
    PIT_TYPE_GIANT = 2,
    PIT_TYPE_LOVECRAFTIAN = 3,
    PIT_TYPE_SYMBOL_GOOD = 4,
    PIT_TYPE_SYMBOL_EVIL = 5,
    PIT_TYPE_CHAPEL = 6,
    PIT_TYPE_DRAGON = 7,
    PIT_TYPE_DEMON = 8,
    PIT_TYPE_DARK_ELF = 9,
};

/*! pit/nest型情報の構造体定義 */
class PlayerType;
struct nest_pit_type {
    concptr name; //<! 部屋名
    bool (*hook_func)(PlayerType *player_ptr, MonsterRaceId r_idx); //<! モンスターフィルタ関数
    void (*prep_func)(PlayerType *player_ptr); //<! 能力フィルタ関数
    DEPTH level; //<! 相当階
    int chance; //!< 生成確率
};

/*! デバッグ時にnestのモンスター情報を確認するための構造体 / A struct for nest monster information with cheat_hear */
struct nest_mon_info_type {
    MonsterRaceId r_idx; //!< モンスター種族ID
    bool used; //!< 既に選んだかどうか
};

struct dun_data_type;
bool build_type5(PlayerType *player_ptr, dun_data_type *dd_ptr);
bool build_type6(PlayerType *player_ptr, dun_data_type *dd_ptr);
bool build_type13(PlayerType *player_ptr, dun_data_type *dd_ptr);
