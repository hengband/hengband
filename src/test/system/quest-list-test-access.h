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
        fixed_maps.base_legend.swap(saved_legend);
    }

    QuestListTestAccess(const QuestListTestAccess &) = delete;
    QuestListTestAccess &operator=(const QuestListTestAccess &) = delete;

    ~QuestListTestAccess()
    {
        quests.quests.swap(saved_quests);
        fixed_maps.maps.swap(saved_maps);
        fixed_maps.base_legend.swap(saved_legend);
    }

    void load(const std::filesystem::path &directory)
    {
        quests.load_json_quests(directory);
    }

    void load_base_legend()
    {
        fixed_maps.set_base_legend(quests.load_base_legend());
    }

    void seed(QuestId id, const QuestType &quest, const QuestFixedMap &fixed_map)
    {
        quests.quests.insert_or_assign(id, quest);
        fixed_maps.maps.insert_or_assign(id, fixed_map);
    }

private:
    QuestList &quests;
    QuestFixedMapList &fixed_maps;
    std::map<QuestId, QuestType> saved_quests;
    std::map<QuestId, QuestFixedMap> saved_maps;
    std::map<char, QuestLegendCell> saved_legend;
};
}
