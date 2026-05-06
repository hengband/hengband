#pragma once

#include "util/enum-range.h"
#include "util/probability-table.h"
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

//!< @details 数値はレアリティの重み。Lowが一番出やすく、Highが一番出にくい.
enum class RumorRarity {
    LOW = 3,
    MEDIUM = 2,
    HIGH = 1,
    MAX,
};

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

    std::map<RumorRarity, std::map<RumorType, std::vector<RumorDefinition>>> rumor_definitions;
    std::map<RumorType, std::string_view> type_template;
    ProbabilityTable<int> rumor_tables;
    std::vector<RumorDefinition> random_rumors;

    const RumorDefinition &select_low_rumor(const std::map<RumorType, std::vector<RumorDefinition>> &rumors_map) const;
    const RumorDefinition &select_medium_rumor(const std::map<RumorType, std::vector<RumorDefinition>> &rumors_map) const;
    const RumorDefinition &select_high_rumor(const std::map<RumorType, std::vector<RumorDefinition>> &rumors_map) const;
};
