#include "system/dungeon/quest-definition.h"
#include "artifact/fixed-art-types.h"
#include "info-reader/fixed-map-parser.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-list.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"

std::vector<std::string> quest_text_lines; /*!< Quest text */
QuestId leaving_quest = QuestId::NONE;

/*!
 * @brief 該当IDが固定クエストかどうかを判定する.
 * @param quest_id クエストID
 * @return 固定クエストならばTRUEを返す
 */
bool QuestType::is_fixed(QuestId quest_id)
{
    return (enum2i(quest_id) < MIN_RANDOM_QUEST) || (enum2i(quest_id) > MAX_RANDOM_QUEST);
}

void QuestType::reset()
{
    this->status = QuestStatusType::UNTAKEN;
    this->cur_num = 0;
    this->max_num = 0;
    this->type = QuestKindType::NONE;
    this->level = 0;
    this->r_idx = MonraceList::empty_id();
    this->complev = 0;
    this->comptime = 0;
}

bool QuestType::has_reward() const
{
    return this->reward_fa_id.has_value();
}

tl::optional<FixedArtifactId> QuestType::get_reward() const
{
    return this->reward_fa_id;
}

short QuestType::get_reward_bi_id() const
{
    if (!this->has_reward()) {
        return 0;
    }

    const auto &artifact = ArtifactList::get_instance().get_artifact(*this->reward_fa_id);
    return BaseitemList::get_instance().lookup_baseitem_id(artifact.bi_key);
}

bool QuestType::is_reward_instant_artifact() const
{
    if (!this->has_reward()) {
        return false;
    }

    return ArtifactList::get_instance().get_artifact(*this->reward_fa_id).is_instant_artifact();
}

bool QuestType::is_reward_target(const BaseitemKey &key) const
{
    if (!this->has_reward()) {
        return false;
    }

    return ArtifactList::get_instance().get_artifact(*this->reward_fa_id).bi_key == key;
}

void QuestType::set_reward(FixedArtifactId fa_id)
{
    if (fa_id == FixedArtifactId::NONE) {
        this->reset_reward();
        return;
    }

    if (this->reward_fa_id && (*this->reward_fa_id != fa_id)) {
        ArtifactRecords::get_instance().set_quest_reward(*this->reward_fa_id, false);
    }

    this->reward_fa_id = fa_id;
    ArtifactRecords::get_instance().set_quest_reward(fa_id, true);
}

void QuestType::reset_reward()
{
    if (this->reward_fa_id) {
        ArtifactRecords::get_instance().set_quest_reward(*this->reward_fa_id, false);
        this->reward_fa_id.reset();
    }
}

/*!
 * @brief 討伐対象モンスターを返す. いなければプレイヤー (無効値の意)
 * @return 討伐対象モンスター
 */
MonraceDefinition &QuestType::get_bounty()
{
    return MonraceList::get_instance().get_monrace(this->r_idx);
}

/*!
 * @brief 討伐対象モンスターを返す. いなければプレイヤー (無効値の意)
 * @return 討伐対象モンスター
 */
const MonraceDefinition &QuestType::get_bounty() const
{
    return MonraceList::get_instance().get_monrace(this->r_idx);
}
