/*!
 * @brief 地形特性の集合論的処理実装
 * @author Hourier
 * @date 2024/12/08
 */

#include "system/terrain/terrain-list.h"
#include "info-reader/feature-info-tokens-table.h"
#include "system/terrain/terrain-definition.h"
#include <algorithm>

TerrainList TerrainList::instance{};

TerrainList &TerrainList::get_instance()
{
    return instance;
}

TerrainType &TerrainList::get_terrain(short terrain_id)
{
    return this->terrains.at(terrain_id);
}

const TerrainType &TerrainList::get_terrain(short terrain_id) const
{
    return this->terrains.at(terrain_id);
}

TerrainType &TerrainList::get_terrain(TerrainTag tag)
{
    return this->terrains.at(this->tags.at(tag));
}

const TerrainType &TerrainList::get_terrain(TerrainTag tag) const
{
    return this->terrains.at(this->tags.at(tag));
}

short TerrainList::get_terrain_id(TerrainTag tag) const
{
    return this->tags.at(tag);
}

/*!
 * @brief 地形タグからIDを得る
 * @param tag タグ文字列
 * @throw std::runtime_error 未定義のタグが指定された
 * @return 地形タグに対応するID
 */
short TerrainList::get_terrain_id_by_tag(std::string_view tag) const
{
    const auto it = std::find_if(this->terrains.begin(), this->terrains.end(),
        [tag](const auto &terrain) {
            return terrain.tag == tag;
        });
    if (it == this->terrains.end()) {
        THROW_EXCEPTION(std::runtime_error, format(_("未定義のタグ '%s'。", "%s is undefined."), tag.data()));
    }

    return static_cast<short>(std::distance(this->terrains.begin(), it));
}

/*!
 * @brief 地形情報の各種タグからIDへ変換して結果を収める
 */
void TerrainList::retouch()
{
    for (auto &terrain : this->terrains) {
        terrain.mimic = this->search_real_terrain(terrain.mimic_tag).value_or(terrain.mimic);
        terrain.destroyed = this->search_real_terrain(terrain.destroyed_tag).value_or(terrain.destroyed);
        for (auto &ts : terrain.state) {
            ts.result = this->search_real_terrain(ts.result_tag).value_or(ts.result);
        }
    }
}

void TerrainList::emplace_tag(std::string_view tag)
{
    this->tags.emplace(terrain_tags.at(tag), this->get_terrain_id_by_tag(tag));
}

/*!
 * @brief 地形タグからIDを得る
 * @param tag タグ文字列のオフセット
 * @return 地形ID。該当がないならstd::nullopt
 */
std::optional<short> TerrainList::search_real_terrain(std::string_view tag) const
{
    if (tag.empty()) {
        return std::nullopt;
    }

    return this->get_terrain_id_by_tag(tag);
}
