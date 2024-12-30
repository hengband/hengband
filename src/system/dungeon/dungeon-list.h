/*!
 * @brief ダンジョンにおける集合論的モデルの定義
 * @author Hourier
 * @date 2024/12/01
 */

#pragma once

#include "util/abstract-map-wrapper.h"
#include <map>
#include <memory>

enum class DungeonId;
class DungeonDefinition;
class DungeonList : public util::AbstractMapWrapper<DungeonId, std::shared_ptr<DungeonDefinition>> {
public:
    DungeonList(DungeonList &&) = delete;
    DungeonList(const DungeonList &) = delete;
    DungeonList &operator=(const DungeonList &) = delete;
    DungeonList &operator=(DungeonList &&) = delete;
    ~DungeonList() = default;

    static DungeonList &get_instance();
    DungeonDefinition &get_dungeon(DungeonId dungeon_id);
    const DungeonDefinition &get_dungeon(DungeonId dungeon_id) const;
    std::shared_ptr<DungeonDefinition> get_dungeon_shared(DungeonId dungeon_id);
    std::shared_ptr<const DungeonDefinition> get_dungeon_shared(DungeonId dungeon_id) const;
    void emplace(DungeonId dungeon_id, DungeonDefinition &&definition);
    void retouch();

private:
    DungeonList() = default;
    static DungeonList instance;

    std::map<DungeonId, std::shared_ptr<DungeonDefinition>> dungeons;

    std::map<DungeonId, std::shared_ptr<DungeonDefinition>> &get_inner_container() override
    {
        return this->dungeons;
    }
};
