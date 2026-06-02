#include "system/dungeon/quest-list.h"
#include "io/files-util.h"
#include "system/angband-exceptions.h"
#include "system/dungeon/quest-definition.h"
#include "system/monrace/monrace-definition.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include <algorithm>
#include <fmt/format.h>
#include <fstream>
#include <set>
#include <string>
#include <string_view>

namespace {
std::ifstream create_ifs_quest_list(std::string_view file_name)
{
    const auto path = path_build(ANGBAND_DIR_EDIT, file_name);
    std::ifstream ifs(path);
    if (ifs) {
        return ifs;
    }

    constexpr auto fmt = _("ファイルが見つかりません ({})", "File is not found ({})");
    THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, file_name));
}

class QuestListParser {
public:
    QuestListParser() = default;
    std::set<QuestId> parse(std::string_view file_name)
    {
        this->key_list.clear();
        this->parse_recursively(file_name);
        return this->key_list;
    }

private:
    std::set<QuestId> key_list;

    void parse_recursively(std::string_view file_name);
};

void QuestListParser::parse_recursively(std::string_view file_name)
{
    auto ifs = create_ifs_quest_list(file_name);
    std::string line;
    auto line_num = 0;
    while (std::getline(ifs, line)) {
        line_num++;
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        line = utf8_to_local(line);
        const auto tokens = str_split(line, ':', true);
        if (tokens.empty()) {
            continue;
        }

        switch (tokens[0][0]) {
        case 'Q': {
            if (tokens.size() < 3) {
                continue;
            }

            const auto is_quest_none = _(tokens[1].starts_with('$'), !tokens[1].starts_with('$')) || (tokens[2] != "N");
            const auto quest_number = is_quest_none ? QuestId::NONE : i2enum<QuestId>(std::stoi(tokens[1].substr(_(0, 1))));
            if (quest_number == QuestId::NONE) {
                break;
            }

            if (this->key_list.find(quest_number) != this->key_list.end()) {
                constexpr auto fmt = _("重複したQuestID {} ({}の{}行目)", "Duplicated Quest Id {} ({} line {})");
                THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, enum2i(quest_number), file_name, line_num));
            }

            this->key_list.insert(quest_number);
            break;
        }
        case '%': {
            if (tokens.size() < 2) {
                continue;
            }

            this->parse_recursively(tokens[1]);
            break;
        }
        default:
            break;
        }
    }
}
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
    const auto quest_numbers = QuestListParser().parse(QUEST_DEFINITION_LIST);
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

void QuestList::set_reward(QuestId id, FixedArtifactId fa_id)
{
    this->quests.at(id).set_reward(fa_id);
}

void QuestList::reset_reward(QuestId id)
{
    this->quests.at(id).reset_reward();
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
