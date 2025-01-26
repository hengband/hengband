#pragma once

#include <map>
#include <optional>
#include <string>

enum class NestKind {
    CLONE = 0,
    JELLY = 1,
    SYMBOL_GOOD = 2,
    SYMBOL_EVIL = 3,
    MIMIC = 4,
    HORROR = 5,
    KENNEL = 6,
    ANIMAL = 7,
    CHAPEL = 8,
    UNDEAD = 9,
    MAX,
};

enum class PitKind {
    ORC = 0,
    TROLL = 1,
    GIANT = 2,
    HORROR = 3,
    SYMBOL_GOOD = 4,
    SYMBOL_EVIL = 5,
    CHAPEL = 6,
    DRAGON = 7,
    DEMON = 8,
    DARK_ELF = 9,
    MAX,
};

enum class PitNestHook {
    NONE,
    CLONE,
    DRAGON,
    SYMBOL,
};

/*! pit/nest型情報の構造体定義 */
enum class MonraceHook;
enum class MonraceId : short;
class PlayerType;
struct nest_pit_type {
    std::string name; //<! 部屋名
    MonraceHook monrace_hook; //<! モンスター種別フィルタ
    PitNestHook pn_hook; //<! シンボル/能力フィルタ
    int level; //<! 相当階
    int chance; //!< 生成確率

    void prepare_filter(PlayerType *player_ptr) const;
};

/*! デバッグ時にnestのモンスター情報を確認するための構造体 / A struct for nest monster information with cheat_hear */
class MonraceDefinition;
class NestMonsterInfo {
public:
    NestMonsterInfo() = default;
    MonraceId monrace_id{}; //!< モンスター種族ID
    bool used = false; //!< 既に選んだかどうか
    bool order_nest(const NestMonsterInfo &other) const;
    const MonraceDefinition &get_monrace() const;
};

class FloorType;
class MonsterEntity;
std::optional<NestKind> pick_nest_type(const FloorType &floor, const std::map<NestKind, nest_pit_type> &np_types);
std::optional<PitKind> pick_pit_type(const FloorType &floor, const std::map<PitKind, nest_pit_type> &np_types);
std::optional<MonraceId> select_pit_nest_monrace_id(PlayerType *player_ptr, MonsterEntity &align, int boost);
