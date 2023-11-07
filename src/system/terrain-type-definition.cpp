/*
 * @brief 地形特性定義
 * @author Hourier
 * @date 2022/10/15
 */

#include "system/terrain-type-definition.h"

std::vector<TerrainType> terrains_info;

bool TerrainType::is_permanent_wall() const
{
    return this->flags.has_all_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::PERMANENT });
}

TerrainList TerrainList::instance{};

TerrainList &TerrainList::get_instance()
{
    return instance;
}

TerrainType &TerrainList::operator[](short terrain_id)
{
    return terrains_info.at(terrain_id);
}

const TerrainType &TerrainList::operator[](short terrain_id) const
{
    return terrains_info.at(terrain_id);
}

std::vector<TerrainType>::iterator TerrainList::begin()
{
    return terrains_info.begin();
}

const std::vector<TerrainType>::const_iterator TerrainList::begin() const
{
    return terrains_info.begin();
}

std::vector<TerrainType>::iterator TerrainList::end()
{
    return terrains_info.end();
}

const std::vector<TerrainType>::const_iterator TerrainList::end() const
{
    return terrains_info.end();
}

size_t TerrainList::size() const
{
    return terrains_info.size();
}

bool TerrainList::empty() const
{
    return terrains_info.empty();
}

void TerrainList::resize(size_t new_size)
{
    terrains_info.resize(new_size);
}
