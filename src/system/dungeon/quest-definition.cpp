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

QuestList QuestList::instance{};

QuestList &QuestList::get_instance()
{
    return instance;
}

/*!
 * @brief クエストの初期化
 * @details ソフトウェア起動時ではパース関数が動作しないので、各種初期化シーケンス時に遅延初期化する
 */
void QuestList::initialize()
{
    const auto quest_numbers = parse_quest_info(QUEST_DEFINITION_LIST);
    QuestType quest{};
    quest.status = QuestStatusType::UNTAKEN;
    this->quests.emplace(QuestId::NONE, quest);
    for (const auto q : quest_numbers) {
        this->quests.emplace(q, quest);
    }
}

void QuestList::reset_all()
{
    for (auto &[_, quest] : this->quests) {
        quest.reset();
    }
}

QuestType &QuestList::get_quest(QuestId id)
{
    return this->quests.at(id);
}

const QuestType &QuestList::get_quest(QuestId id) const
{
    return this->quests.at(id);
}

std::vector<QuestId> QuestList::get_sorted_quest_ids() const
{
    std::vector<QuestId> quest_ids;
    std::transform(++this->quests.begin(), this->quests.end(), std::back_inserter(quest_ids), [](const auto &x) { return x.first; });
    std::stable_sort(quest_ids.begin(), quest_ids.end(), [this](auto x, auto y) { return this->order_completed(x, y); });
    return quest_ids;
}

void QuestList::set_defeated_monster(QuestId id, short numbers)
{
    this->quests.at(id).cur_num = numbers;
}

void QuestList::set_max_monster(QuestId id, short numbers)
{
    this->quests.at(id).max_num = numbers;
}

void QuestList::set_type(QuestId id, QuestKindType type)
{
    this->quests.at(id).type = type;
}

void QuestList::set_monrace_id(QuestId id, MonraceId monrace_id)
{
    this->quests.at(id).r_idx = monrace_id;
}

void QuestList::set_flags(QuestId id, uint32_t flags)
{
    this->quests.at(id).flags = flags;
}

bool QuestList::is_quest_equals(QuestId id, QuestKindType type) const
{
    return this->quests.at(id).type == type;
}

bool QuestList::is_bounty_valid(QuestId id) const
{
    return this->quests.at(id).get_bounty().is_valid();
}

bool QuestList::order_completed(QuestId id1, QuestId id2) const
{
    const auto &quest1 = this->get_quest(id1);
    const auto &quest2 = this->get_quest(id2);
    return (quest1.comptime != quest2.comptime) ? (quest1.comptime < quest2.comptime) : (quest1.level < quest2.level);
}
