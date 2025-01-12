#include "system/dungeon/dungeon-definition.h"
#include "dungeon/dungeon-flag-mask.h"
#include "grid/feature.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/z-form.h"

enum conversion_type {
    CONVERT_TYPE_FLOOR = 0,
    CONVERT_TYPE_WALL = 1,
    CONVERT_TYPE_INNER = 2,
    CONVERT_TYPE_OUTER = 3,
    CONVERT_TYPE_SOLID = 4,
    CONVERT_TYPE_STREAM1 = 5,
    CONVERT_TYPE_STREAM2 = 6,
};

bool DungeonDefinition::has_river_flag() const
{
    return this->flags.has_any_of(DF_RIVER_MASK);
}

bool DungeonDefinition::has_guardian() const
{
    return this->final_guardian != MonraceId::PLAYER;
}

MonraceDefinition &DungeonDefinition::get_guardian()
{
    return MonraceList::get_instance().get_monrace(this->final_guardian);
}

const MonraceDefinition &DungeonDefinition::get_guardian() const
{
    return MonraceList::get_instance().get_monrace(this->final_guardian);
}

/*
 * @brief 地形とそれへのアクションから新しい地形IDを返す
 * @param terrain_id 元の地形ID
 * @param action 地形特性
 * @return 新しい地形ID
 */
short DungeonDefinition::convert_terrain_id(short terrain_id, TerrainCharacteristics action) const
{
    const auto &terrain = TerrainList::get_instance().get_terrain(terrain_id);
    for (auto i = 0; i < MAX_FEAT_STATES; i++) {
        if (terrain.state[i].action == action) {
            return this->convert_terrain_id(terrain.state[i].result);
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::PERMANENT)) {
        return terrain_id;
    }

    const auto has_action_flag = TerrainType::has(action, TerrainAction::DESTROY);
    return has_action_flag ? this->convert_terrain_id(terrain.destroyed) : terrain_id;
}

short DungeonDefinition::convert_terrain_id(short terrain_id) const
{
    const auto &terrain = TerrainList::get_instance().get_terrain(terrain_id);
    if (terrain.flags.has_not(TerrainCharacteristics::CONVERT)) {
        return terrain_id;
    }

    switch (terrain.subtype) {
    case CONVERT_TYPE_FLOOR:
        return rand_choice(feat_ground_type);
    case CONVERT_TYPE_WALL:
        return rand_choice(feat_wall_type);
    case CONVERT_TYPE_INNER:
        return feat_wall_inner;
    case CONVERT_TYPE_OUTER:
        return feat_wall_outer;
    case CONVERT_TYPE_SOLID:
        return feat_wall_solid;
    case CONVERT_TYPE_STREAM1:
        return this->stream1;
    case CONVERT_TYPE_STREAM2:
        return this->stream2;
    default:
        return terrain_id;
    }
}

/*!
 * @brief ドアやカーテンが開いているかを調べる
 * @param terrain_id 地形情報のID
 * @return 開いているか否か
 * @details 上流で地形側の判定と併せて判定すること
 */
bool DungeonDefinition::is_open(short terrain_id) const
{
    return terrain_id != this->convert_terrain_id(terrain_id, TerrainCharacteristics::CLOSE);
}

/*!
 * @brief ダンジョンを制覇したかを返す
 * @return そもそもダンジョンの主がいなかったらfalse、主がいるならば主を撃破したか否か
 */
bool DungeonDefinition::is_conquered() const
{
    if (!this->has_guardian()) {
        return false;
    }

    return this->get_guardian().is_dead_unique();
}

std::string DungeonDefinition::build_entrance_message() const
{
    constexpr auto fmt = _("ここには%sの入り口(%d階相当)があります", "There is the entrance of %s (Danger level: %d)");
    return format(fmt, this->name.data(), this->mindepth);
}

std::string DungeonDefinition::describe_depth() const
{
    return format(_("%s(%d階相当)", "%s(level %d)"), this->text.data(), this->mindepth);
}

/*
 * @brief 洞窟らしい地形 (湖、溶岩、瓦礫、森林)の個数を決める
 * @return 地形の個数
 */
int DungeonDefinition::calc_cavern_terrains() const
{
    auto count = 0;
    if (this->flags.has(DungeonFeatureType::LAKE_WATER)) {
        count += 3;
    }

    if (this->flags.has(DungeonFeatureType::LAKE_LAVA)) {
        count += 3;
    }

    if (this->flags.has(DungeonFeatureType::LAKE_RUBBLE)) {
        count += 3;
    }

    if (this->flags.has(DungeonFeatureType::LAKE_TREE)) {
        count += 3;
    }

    return count;
}

void DungeonDefinition::set_guardian_flag()
{
    if (this->has_guardian()) {
        auto &monrace = this->get_guardian();
        monrace.misc_flags.set(MonsterMiscType::GUARDIAN);
    }
}
