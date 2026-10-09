#pragma once

#include "util/abstract-map-wrapper.h"
#include <cstdint>
#include <filesystem>
#include <map>
#include <tl/optional.hpp>
#include <vector>

enum class FixedArtifactId : short;
enum class MonraceId : short;
enum class QuestId : short;
enum class QuestKindType : short;
class QuestType;
struct QuestFixedMap;
struct QuestLegendCell;
namespace test {
class QuestListTestAccess;
class QuestFeatureTestAccess;
}
class QuestList final : public util::AbstractMapWrapper<QuestId, QuestType> {
public:
    QuestList(const QuestList &) = delete;
    QuestList(QuestList &&) = delete;
    QuestList &operator=(const QuestList &) = delete;
    QuestList &operator=(QuestList &&) = delete;
    static QuestList &get_instance();

    void initialize();
    void reset_all();
    QuestType &get_quest(QuestId id);
    const QuestType &get_quest(QuestId id) const;
    std::vector<QuestId> get_sorted_quest_ids() const;
    tl::optional<QuestId> find_shallowest_random_quest_id() const;

    void set_defeated_monster(QuestId id, short numbers);
    void set_max_monster(QuestId id, short numbers);
    void set_type(QuestId id, QuestKindType type);
    void set_monrace_id(QuestId id, MonraceId monrace_id);
    void set_flags(QuestId id, uint32_t flags);
    void set_reward(QuestId id, FixedArtifactId fa_id);
    void reset_reward(QuestId id);
    bool is_quest_equals(QuestId id, QuestKindType type) const;
    bool is_bounty_valid(QuestId id) const;

private:
    friend class test::QuestListTestAccess;
    friend class test::QuestFeatureTestAccess;

    static QuestList instance;
    std::map<QuestId, QuestType> quests;
    QuestList() = default;

    std::map<char, QuestLegendCell> load_base_legend(); //!< 共通ベース凡例を公開せずに読み込む
    // テスト用経路: 既存ストアのコピーへ読み込み、検証後にクエストと固定マップを公開する。
    void load_json_quests(const std::filesystem::path &quests_dir);
    // 指定された一時ストアに読み込み、検証とメタデータ適用を行う。ストア自体は公開しない。
    void load_json_quests(const std::filesystem::path &quests_dir, std::map<QuestId, QuestType> &parsed_quests, std::map<QuestId, QuestFixedMap> &parsed_maps);
    void publish(std::map<QuestId, QuestType> &parsed_quests, std::map<QuestId, QuestFixedMap> &parsed_maps) noexcept;

    std::map<QuestId, QuestType> &get_inner_container() override
    {
        return this->quests;
    }

    bool order_completed(QuestId id1, QuestId id2) const;
};
