#pragma once

#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include <filesystem>
#include <map>

namespace test {
class QuestListTestAccess {
public:
    QuestListTestAccess()
        : quests(QuestList::get_instance())
        , fixed_maps(QuestFixedMapList::get_instance())
    {
        quests.quests.swap(saved_quests);
        fixed_maps.maps.swap(saved_maps);
    }

    QuestListTestAccess(const QuestListTestAccess &) = delete;
    QuestListTestAccess &operator=(const QuestListTestAccess &) = delete;

    ~QuestListTestAccess()
    {
        quests.quests.swap(saved_quests);
        fixed_maps.maps.swap(saved_maps);
    }

    void load(const std::filesystem::path &directory)
    {
        quests.load_json_quests(directory);
    }

    void load_base_legend()
    {
        quests.load_base_legend();
    }

private:
    QuestList &quests;
    QuestFixedMapList &fixed_maps;
    std::map<QuestId, QuestType> saved_quests;
    std::map<QuestId, QuestFixedMap> saved_maps;
};
}
