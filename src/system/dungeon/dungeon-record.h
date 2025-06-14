/*!
 * @brief ダンジョンに関するプレイ記録定義
 * @author Hourier
 * @date 2024/12/01
 */

#pragma once

#include "util/abstract-map-wrapper.h"
#include <map>
#include <memory>
#include <string>
#include <tl/optional.hpp>
#include <vector>

class DungeonRecord {
public:
    DungeonRecord() = default;
    bool has_entered() const;
    int get_max_level() const;
    int get_max_max_level() const;
    void set_max_level(int level);
    void reset();

private:
    tl::optional<int> max_max_level; //!< @details 将来の拡張. 帰還時に浅いフロアを指定しても維持する.
    tl::optional<int> max_level; //!< @details 帰還時に浅いフロアを指定すると書き換わる.
};

enum class DungeonId;
class DungeonDefinition;
class DungeonRecords : public util::AbstractMapWrapper<DungeonId, std::shared_ptr<DungeonRecord>> {
public:
    DungeonRecords(DungeonRecords &&) = delete;
    DungeonRecords(const DungeonRecords &) = delete;
    DungeonRecords &operator=(const DungeonRecords &) = delete;
    DungeonRecords &operator=(DungeonRecords &&) = delete;
    ~DungeonRecords() = default;

    static DungeonRecords &get_instance();
    DungeonRecord &get_record(DungeonId dungeon_id);
    const DungeonRecord &get_record(DungeonId dungeon_id) const;
    std::shared_ptr<DungeonRecord> get_record_shared(DungeonId dungeon_id);
    std::shared_ptr<const DungeonRecord> get_record_shared(DungeonId dungeon_id) const;
    void reset_all();

    std::vector<DungeonId> collect_entered_dungeon_ids() const;

private:
    DungeonRecords();
    static DungeonRecords instance;
    std::map<DungeonId, std::shared_ptr<DungeonRecord>> records;

    std::map<DungeonId, std::shared_ptr<DungeonRecord>> &get_inner_container() override
    {
        return this->records;
    }
};
