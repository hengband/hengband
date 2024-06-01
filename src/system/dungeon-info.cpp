#include "system/dungeon-info.h"
#include "dungeon/dungeon-flag-mask.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "system/monster-race-info.h"

/*
 * The dungeon arrays
 */
std::vector<dungeon_type> dungeons_info;

/*
 * Maximum number of dungeon in DungeonDefinitions.txt
 */
std::vector<DEPTH> max_dlv;

bool dungeon_type::has_river_flag() const
{
    return this->flags.has_any_of(DF_RIVER_MASK);
}

/*!
 * @brief ダンジョンが地下ダンジョンかを判定する
 * @return 地下ダンジョンならtrue、地上 (荒野)ならfalse
 */
bool dungeon_type::is_dungeon() const
{
    return this->idx > 0;
}

bool dungeon_type::has_guardian() const
{
    return this->final_guardian != MonsterRaceId::PLAYER;
}

MonsterRaceInfo &dungeon_type::get_guardian()
{
    return monraces_info[this->final_guardian];
}

const MonsterRaceInfo &dungeon_type::get_guardian() const
{
    return monraces_info[this->final_guardian];
}
