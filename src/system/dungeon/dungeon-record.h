/*!
 * @brief ダンジョンに関するプレイ記録定義
 * @author Hourier
 * @date 2024/12/01
 */

#pragma once

#include <map>
#include <optional>
#include <vector>

extern std::vector<int> max_dlv;

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

class DungeonRecords {
public:
    DungeonRecords(DungeonRecords &&) = delete;
    DungeonRecords(const DungeonRecords &) = delete;
    DungeonRecords &operator=(const DungeonRecords &) = delete;
    DungeonRecords &operator=(DungeonRecords &&) = delete;
    ~DungeonRecords() = default;

    static DungeonRecords &get_instance();
    DungeonRecord &get_record(int dungeon_id);
    const DungeonRecord &get_record(int dungeon_id) const;
    std::map<int, DungeonRecord>::iterator begin();
    std::map<int, DungeonRecord>::const_iterator begin() const;
    std::map<int, DungeonRecord>::iterator end();
    std::map<int, DungeonRecord>::const_iterator end() const;
    std::map<int, DungeonRecord>::reverse_iterator rbegin();
    std::map<int, DungeonRecord>::const_reverse_iterator rbegin() const;
    std::map<int, DungeonRecord>::reverse_iterator rend();
    std::map<int, DungeonRecord>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void reset_all();

private:
    DungeonRecords();
    static DungeonRecords instance;
    std::map<int, DungeonRecord> records;
};
