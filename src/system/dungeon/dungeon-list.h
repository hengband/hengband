/*!
 * @brief ダンジョンにおける集合論的モデルの定義
 * @author Hourier
 * @date 2024/12/01
 */

#pragma once

#include <map>
#include <memory>

enum class DungeonId;
class DungeonDefinition;
class DungeonList {
public:
    DungeonList(DungeonList &&) = delete;
    DungeonList(const DungeonList &) = delete;
    DungeonList &operator=(const DungeonList &) = delete;
    DungeonList &operator=(DungeonList &&) = delete;
    ~DungeonList() = default;

    static DungeonList &get_instance();
    DungeonDefinition &get_dungeon(DungeonId dungeon_id);
    const DungeonDefinition &get_dungeon(DungeonId dungeon_id) const;
    std::shared_ptr<DungeonDefinition> get_dungeon_shared(DungeonId dungeon_id) const;
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::iterator begin();
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_iterator begin() const;
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::iterator end();
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_iterator end() const;
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::reverse_iterator rbegin();
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_reverse_iterator rbegin() const;
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::reverse_iterator rend();
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void emplace(DungeonId dungeon_id, DungeonDefinition &&definition);
    void retouch();

private:
    DungeonList() = default;
    static DungeonList instance;

    std::map<DungeonId, std::shared_ptr<DungeonDefinition>> dungeons;
};
