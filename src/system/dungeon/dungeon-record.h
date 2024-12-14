/*!
 * @brief ダンジョンに関するプレイ記録定義
 * @author Hourier
 * @date 2024/12/01
 */

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class DungeonMessageFormat {
    DUMP,
    KNOWLEDGE,
    RECALL,
};

class DungeonRecord {
public:
    DungeonRecord() = default;
    bool has_entered() const;
    int get_max_level() const;
    int get_max_max_level() const;
    void set_max_level(int level);
    void reset();

private:
    std::optional<int> max_max_level; //!< @details 将来の拡張. 帰還時に浅いフロアを指定しても維持する.
    std::optional<int> max_level; //!< @details 帰還時に浅いフロアを指定すると書き換わる.
};

enum class DungeonId;
class DungeonDefinition;
class DungeonRecords {
public:
    DungeonRecords(DungeonRecords &&) = delete;
    DungeonRecords(const DungeonRecords &) = delete;
    DungeonRecords &operator=(const DungeonRecords &) = delete;
    DungeonRecords &operator=(DungeonRecords &&) = delete;
    ~DungeonRecords() = default;

    static DungeonRecords &get_instance();
    DungeonRecord &get_record(DungeonId dungeon_id);
    const DungeonRecord &get_record(DungeonId dungeon_id) const;
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::iterator begin();
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::const_iterator begin() const;
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::iterator end();
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::const_iterator end() const;
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::reverse_iterator rbegin();
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::const_reverse_iterator rbegin() const;
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::reverse_iterator rend();
    std::map<DungeonId, std::shared_ptr<DungeonRecord>>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void reset_all();

    int find_max_level() const;
    std::vector<std::string> build_known_dungeons(DungeonMessageFormat dmf) const;
    std::vector<DungeonId> collect_entered_dungeon_ids() const;
    std::pair<std::shared_ptr<DungeonRecord>, std::shared_ptr<DungeonDefinition>> get_dungeon_pair(DungeonId dungeon_id) const;

private:
    DungeonRecords();
    static DungeonRecords instance;
    std::map<DungeonId, std::shared_ptr<DungeonRecord>> records;
};
