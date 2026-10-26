/*!
 * @brief 地形特性の集合論的処理定義
 * @author Hourier
 * @date 2024/12/08
 */

#pragma once

#include "util/abstract-vector-wrapper.h"
#include <string_view>
#include <tl/optional.hpp>
#include <vector>

enum class TerrainTag;
class TerrainType;
namespace test {
class TerrainListTestAccess;
}

class TerrainList : public util::AbstractVectorWrapper<TerrainType> {
public:
    TerrainList(const TerrainList &) = delete;
    TerrainList(TerrainList &&) = delete;
    TerrainList operator=(const TerrainList &) = delete;
    TerrainList operator=(TerrainList &&) = delete;
    ~TerrainList() override;

    static TerrainList &get_instance();
    TerrainType &get_terrain(short terrain_id);
    const TerrainType &get_terrain(short terrain_id) const;
    TerrainType &get_terrain(TerrainTag tag);
    const TerrainType &get_terrain(TerrainTag tag) const;
    short get_terrain_id(TerrainTag tag) const;
    short get_terrain_id(std::string_view tag) const;
    TerrainTag select_normal_trap() const;

    void retouch();
    void emplace_tags();

private:
    friend class test::TerrainListTestAccess;

    TerrainList();

    static TerrainList instance;
    std::vector<TerrainType> terrains;
    std::vector<short> tags; //!< 地形タグの値を添字とした地形ID (対応する地形の無いタグは UNASSIGNED_TERRAIN_ID)
    std::vector<TerrainTag> normal_traps;

    std::vector<TerrainType> &get_inner_container() override
    {
        return this->terrains;
    }

    static constexpr short UNASSIGNED_TERRAIN_ID = -1;

    tl::optional<short> search_real_terrain(std::string_view tag) const;
    void set_terrain_id(TerrainTag tag, short terrain_id);
};
