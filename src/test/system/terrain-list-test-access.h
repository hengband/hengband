#pragma once

#include "system/terrain/terrain-list.h"
#include <map>

namespace test {

/*!
 * @brief 地形タグの対応を一時的に差し替え、スコープ終了時に元の対応表へ戻す
 */
class TerrainListTestAccess {
public:
    TerrainListTestAccess(TerrainTag tag, short terrain_id)
        : terrains(TerrainList::get_instance())
        , previous_tags(terrains.tags)
    {
        auto test_tags = previous_tags;
        test_tags[tag] = terrain_id;
        terrains.tags.swap(test_tags);
    }

    TerrainListTestAccess(const TerrainListTestAccess &) = delete;
    TerrainListTestAccess &operator=(const TerrainListTestAccess &) = delete;

    ~TerrainListTestAccess()
    {
        terrains.tags.swap(previous_tags);
    }

    /*! @brief 現在の地形タグ対応表を検証用に取得する */
    static std::map<TerrainTag, short> current_tags()
    {
        return TerrainList::get_instance().tags;
    }

private:
    TerrainList &terrains;
    std::map<TerrainTag, short> previous_tags;
};

}
