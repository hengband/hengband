/*!
 * @brief 地形特性の集合論的処理定義
 * @author Hourier
 * @date 2024/12/08
 */

#pragma once

#include <map>
#include <optional>
#include <string_view>
#include <vector>

enum class TerrainTag;
class TerrainType;
class TerrainList {
public:
    TerrainList(const TerrainList &) = delete;
    TerrainList(TerrainList &&) = delete;
    TerrainList operator=(const TerrainList &) = delete;
    TerrainList operator=(TerrainList &&) = delete;

    static TerrainList &get_instance();
    TerrainType &get_terrain(short terrain_id);
    const TerrainType &get_terrain(short terrain_id) const;
    TerrainType &get_terrain(TerrainTag tag);
    const TerrainType &get_terrain(TerrainTag tag) const;
    short get_terrain_id(TerrainTag tag) const;
    short get_terrain_id_by_tag(std::string_view tag) const;
    std::vector<TerrainType>::iterator begin();
    std::vector<TerrainType>::const_iterator begin() const;
    std::vector<TerrainType>::reverse_iterator rbegin();
    std::vector<TerrainType>::const_reverse_iterator rbegin() const;
    std::vector<TerrainType>::iterator end();
    std::vector<TerrainType>::const_iterator end() const;
    std::vector<TerrainType>::reverse_iterator rend();
    std::vector<TerrainType>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void resize(size_t new_size);
    void shrink_to_fit();

    void retouch();
    void emplace_tag(std::string_view tag);

private:
    TerrainList() = default;

    static TerrainList instance;
    std::vector<TerrainType> terrains{};
    std::map<TerrainTag, short> tags; //!< @details 全てのTerrainTag を繰り込んだら、terrains からlookupが可能になる. そうなったら削除する.

    std::optional<short> search_real_terrain(std::string_view tag) const;
};
