#pragma once

/*!
 * @class ArtifactRecord
 * @brief ゲーム中可変な固定アーティファクト情報
 * @author Hourier
 * @date 2026/05/10
 */

#include "util/abstract-map-wrapper.h"
#include <tl/optional.hpp>
#include <vector>

class ArtifactRecord {
public:
    ArtifactRecord() = default;

    const tl::optional<short> &get_floor_id() const;
    bool get_generated() const;
    bool get_identified() const;
    bool get_known() const;
    bool get_quest_reward() const;
    bool can_generate() const;

    void set_floor_id(const tl::optional<short> &id);
    void set_generated(bool new_state);
    void set_identified(bool new_state = true); //!< ニューゲーム時またはウィザードモードでのみfalseをsetする.
    void set_known(bool new_state = true); //!< ウィザードモードでのみfalseをsetする.
    void set_quest_reward(bool new_state);

private:
    tl::optional<short> floor_id = tl::nullopt; //!< アイテムを落としたフロアのID.
    bool is_generated = false; //!< 生成済か否か (生成済でも、「保存モードON」かつ「帰還等で鑑定前に消滅」したら未生成状態に戻る)
    bool is_identified = false; //!< 鑑定 or *鑑定*済か否か (簡易鑑定の段階ではfalse)
    bool is_known = false; //!< 今までに鑑定したり、噂を聞いたことがあるか (前世のセーブも含)
    bool is_quest_reward = false; //!< クエスト報酬か否か (宝物庫用)
};

enum class FixedArtifactId : short;
class ArtifactRecords : public util::AbstractMapWrapper<FixedArtifactId, ArtifactRecord> {
public:
    ArtifactRecords(ArtifactRecords &&) = delete;
    ArtifactRecords(const ArtifactRecords &) = delete;
    ArtifactRecords &operator=(const ArtifactRecords &) = delete;
    ArtifactRecords &operator=(ArtifactRecords &&) = delete;
    ~ArtifactRecords() = default;

    static ArtifactRecords &get_instance();
    void initialize(size_t size);

    const tl::optional<short> &get_floor_id(FixedArtifactId fa_id) const;
    bool get_generated(FixedArtifactId fa_id) const;
    bool get_identified(FixedArtifactId fa_id) const;
    bool get_known(FixedArtifactId fa_id) const;
    bool get_quest_reward(FixedArtifactId fa_id) const;
    bool can_generate(FixedArtifactId fa_id) const;

    void set_floor_id(FixedArtifactId fa_id, const tl::optional<short> &id);
    void set_generated(FixedArtifactId fa_id, bool new_state);
    void set_identified(FixedArtifactId fa_id, bool new_state = true);
    void set_known(FixedArtifactId fa_id, bool new_state = true);
    void set_quest_reward(FixedArtifactId fa_id, bool new_state);
    void reset_all_without_knowledge();

    std::vector<FixedArtifactId> collect_known_ids() const;
    std::vector<FixedArtifactId> collect_identified_ids() const;

private:
    ArtifactRecords() = default;

    static ArtifactRecords instance;

    std::map<FixedArtifactId, ArtifactRecord> records;
    std::map<FixedArtifactId, ArtifactRecord> &get_inner_container() override
    {
        return this->records;
    }

    void validate_fixed_artifact_id(FixedArtifactId fa_id) const;
};
