#pragma once

#include "util/probability-table.h"
#include <filesystem>
#include <map>
#include <memory>
#include <string_view>
#include <vector>

enum class RumorRarity;
enum class RumorType;
class RumorDefinition;
class RumorList {
public:
    RumorList(RumorList &&) = delete;
    RumorList(const RumorList &) = delete;
    RumorList &operator=(const RumorList &) = delete;
    RumorList &operator=(RumorList &&) = delete;

    static RumorList &get_instance();
    const RumorDefinition &select_random_rumor() const;
    const RumorDefinition &select_rumor(RumorRarity rt) const;

    void read_rumors(const std::filesystem::path &path);

    // @details RumorService::retouch() からのみ呼び出される.
    void add_towns();
    void add_shallow_dungeons();
    void add_normal_monsters();
    void add_shallow_artifacts();
    void add_deep_dungeons();
    void add_unique_monsters();
    void add_deep_artifacts();
    void validate() const;
    void make_table();

private:
    RumorList();

    static RumorList instance;

    std::map<RumorRarity, std::map<RumorType, std::vector<std::shared_ptr<const RumorDefinition>>>> rumor_definitions;
    std::map<RumorType, std::string_view> type_template;
    ProbabilityTable<std::shared_ptr<const RumorDefinition>> random_rumors_table;
};
