/*!
 * @brief ダンジョンにおける集合論的モデルの定義
 * @author Hourier
 * @date 2024/12/01
 */

#pragma once

#include <map>
#include <memory>

class DungeonDefinition;
class DungeonList {
public:
    DungeonList(DungeonList &&) = delete;
    DungeonList(const DungeonList &) = delete;
    DungeonList &operator=(const DungeonList &) = delete;
    DungeonList &operator=(DungeonList &&) = delete;
    ~DungeonList() = default;

    static DungeonList &get_instance();
    DungeonDefinition &get_dungeon(int dungeon_id);
    const DungeonDefinition &get_dungeon(int dungeon_id) const;
    std::map<int, std::shared_ptr<DungeonDefinition>>::iterator begin();
    std::map<int, std::shared_ptr<DungeonDefinition>>::const_iterator begin() const;
    std::map<int, std::shared_ptr<DungeonDefinition>>::iterator end();
    std::map<int, std::shared_ptr<DungeonDefinition>>::const_iterator end() const;
    std::map<int, std::shared_ptr<DungeonDefinition>>::reverse_iterator rbegin();
    std::map<int, std::shared_ptr<DungeonDefinition>>::const_reverse_iterator rbegin() const;
    std::map<int, std::shared_ptr<DungeonDefinition>>::reverse_iterator rend();
    std::map<int, std::shared_ptr<DungeonDefinition>>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void emplace(int dungeon_id, DungeonDefinition &&definition);
    void retouch();

private:
    DungeonList() = default;
    static DungeonList instance;

    std::map<int, std::shared_ptr<DungeonDefinition>> dungeons;
};
