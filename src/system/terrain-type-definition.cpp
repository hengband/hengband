/*
 * @brief 地形特性定義
 * @author Hourier
 * @date 2022/10/15
 */

#include "system/terrain-type-definition.h"

bool TerrainType::is_permanent_wall() const
{
    return this->flags.has_all_of({ TerrainCharacteristics::WALL, TerrainCharacteristics::PERMANENT });
}

TerrainList TerrainList::instance{};

TerrainList &TerrainList::get_instance()
{
    return instance;
}

std::vector<TerrainType> &TerrainList::get_raw_vector()
{
    return this->terrains;
}

TerrainType &TerrainList::operator[](short terrain_id)
{
    return this->terrains.at(terrain_id);
}

const TerrainType &TerrainList::operator[](short terrain_id) const
{
    return this->terrains.at(terrain_id);
}

std::vector<TerrainType>::iterator TerrainList::begin()
{
    return this->terrains.begin();
}

std::vector<TerrainType>::const_iterator TerrainList::begin() const
{
    return this->terrains.begin();
}

std::vector<TerrainType>::reverse_iterator TerrainList::rbegin()
{
    return this->terrains.rbegin();
}

std::vector<TerrainType>::const_reverse_iterator TerrainList::rbegin() const
{
    return this->terrains.rbegin();
}

std::vector<TerrainType>::iterator TerrainList::end()
{
    return this->terrains.end();
}

std::vector<TerrainType>::const_iterator TerrainList::end() const
{
    return this->terrains.end();
}

std::vector<TerrainType>::reverse_iterator TerrainList::rend()
{
    return this->terrains.rend();
}

std::vector<TerrainType>::const_reverse_iterator TerrainList::rend() const
{
    return this->terrains.rend();
}

size_t TerrainList::size() const
{
    return this->terrains.size();
}

bool TerrainList::empty() const
{
    return this->terrains.empty();
}

void TerrainList::resize(size_t new_size)
{
    this->terrains.resize(new_size);
}
