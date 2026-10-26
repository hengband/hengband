#pragma once

#include "system/dungeon/dungeon-list.h"

namespace test {

/*! @brief ダンジョン一覧をテスト用に空にし、終了時に既存の参照ごと復元する */
class DungeonListTestAccess {
public:
    DungeonListTestAccess()
        : dungeons(DungeonList::get_instance())
    {
        dungeons.dungeons.swap(previous_dungeons);
    }

    DungeonListTestAccess(const DungeonListTestAccess &) = delete;
    DungeonListTestAccess &operator=(const DungeonListTestAccess &) = delete;

    ~DungeonListTestAccess()
    {
        dungeons.dungeons.swap(previous_dungeons);
    }

private:
    DungeonList &dungeons;
    std::map<DungeonId, std::shared_ptr<DungeonDefinition>> previous_dungeons;
};

}
