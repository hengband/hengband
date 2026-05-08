#include "store/rumor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "locale/language-switcher.h"
#include "rumor/rumor-definition.h"
#include "rumor/rumor-list.h"
#include "system/angband-exceptions.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/town-list.h"
#include "system/floor/town-records.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-record.h"
#include "system/monrace/monrace-records.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace {
void display_town_rumor(const RumorDefinition &rumor)
{
    msg_erase();
    msg_print(rumor.get_description());
    const auto town_id = std::get<TownId>(rumor.get_id());
    auto &town_records = TownRecords::get_instance();
    if (town_records.has_visited(town_id)) {
        return;
    }

    town_records.set_visited(town_id);
    msg_erase();
    constexpr auto fmt = _("{}への行き方が分かった。", "You know the way to {}.");
    msg_print(fmt, TownList::get_instance().get_town(town_id).get_name());
}

void display_dungeon_rumor(const RumorDefinition &rumor)
{
    const auto dungeon_id = std::get<DungeonId>(rumor.get_id());
    msg_erase();
    msg_print(rumor.get_description());
    auto &dungeon_record = DungeonRecords::get_instance().get_record(dungeon_id);
    if (dungeon_record.has_entered()) {
        return;
    }

    const auto &dungeon = DungeonList::get_instance().get_dungeon(dungeon_id);
    dungeon_record.set_max_level(dungeon.mindepth);
    msg_erase();
    constexpr auto fmt = _("{}に帰還できるようになった。", "You can recall to {}.");
    msg_print(fmt, dungeon.name);
}

void display_monster_rumor(const RumorDefinition &rumor)
{
    const auto monrace_id = std::get<MonraceId>(rumor.get_id());
    auto record = MonraceRecords::get_instance().get_record(monrace_id);
    msg_erase();
    msg_print(rumor.get_description());
    if (!record->has_been_seen()) {
        record->increment_seen_count();
    }
}

void display_artifact_rumor(const RumorDefinition &rumor)
{
    msg_erase();
    msg_print(rumor.get_description());
    const auto fa_id = std::get<FixedArtifactId>(rumor.get_id());
    auto &records = ArtifactRecords::get_instance();
    if (records.get_known(fa_id)) {
        return;
    }

    records.set_known(fa_id);
    msg_erase();
    constexpr auto fmt = _("{}の名前を書き留めた。", "You note the name of {}.");
    msg_print(fmt, ArtifactList::get_instance().get_full_name(fa_id));
}

/*!
 * @brief 固定アーティファクト、モンスター、町 をランダムに1つ選ぶ
 * @param zz 検索文字列
 * @param max_idx briefに挙げた各リストにおける最大数
 * @details rumor.txt (rumor_j.txt) の定義により、常にランダム ("*")。但し拡張性のため固定値の場合も残す.
 */
template <typename T, typename U>
T get_rumor_num(std::string_view zz, U max_idx)
    requires(std::is_integral_v<T> || std::is_enum_v<T>) && (std::is_integral_v<U> || std::is_enum_v<U>)
{
    if (zz == "*") {
        return randnum1<T>(max_idx);
    }

    return static_cast<T>(std::atoi(zz.data()));
}

/*!
 * @brief 噂の、町やモンスターを表すトークンを得る
 * @param rumor rumor.txt (rumor_j.txt)の1行
 * @return トークン群の配列を返す。フィールドの数が合わない場合はtl::nulloptを返す。
 * @todo tmp_tokensを使わず単なるsplitにすればもっと簡略化できそう
 */
tl::optional<std::vector<std::string>> get_rumor_tokens(std::string_view rumor)
{
    constexpr auto num_tokens = 3;
    const auto tokens = tokenize(rumor.substr(2), num_tokens);
    if (tokens.size() != num_tokens) {
        msg_print(_("この情報は間違っている。", "This information is wrong."));
        return tl::nullopt;
    }

    return tokens;
}
}

class ArtifactRumor {
public:
    ArtifactRumor(std::span<const std::string> tokens)
    {
        const auto &artifact_id = tokens[1];
        const auto &artifacts = ArtifactList::get_instance();
        this->fa_id = get_rumor_num<FixedArtifactId>(artifact_id, artifacts.rbegin()->first);
        const auto &artifact = artifacts.get_artifact(this->fa_id);
        this->bi_id = BaseitemList::get_instance().lookup_baseitem_id(artifact.bi_key);
    }

    FixedArtifactId fa_id{};
    short bi_id{};
};

class MonsterRumor {
public:
    MonsterRumor(std::span<const std::string> tokens)
    {
        const auto &monster_name = tokens[1];

        // @details プレイヤーもダミーで入っているので、1つ引いておかないと数が合わなくなる.
        auto &monraces = MonraceList::get_instance();
        this->monrace_id = get_rumor_num<MonraceId>(monster_name, monraces.size() - 1);
    }

    MonraceId monrace_id{};
};

class DungeonRumor {
public:
    /// @note tokens[1] の値は正しいことを前提とし、std::stoiの例外処理は行わない
    DungeonRumor(std::span<const std::string> tokens)
        : dungeon_id(i2enum<DungeonId>(std::stoi(tokens[1])))
    {
    }

    DungeonId dungeon_id;
};

class TownRumor {
public:
    TownRumor(std::span<const std::string> tokens)
    {
        const auto &town_name = tokens[1];
        while (true) {
            this->t_idx = get_rumor_num<int>(town_name, VALID_TOWNS);
            if (!TownList::get_instance().get_town(this->t_idx).get_name().empty()) {
                return;
            }
        }
    }

    int t_idx{};
};

using Rumor = std::variant<ArtifactRumor, MonsterRumor, DungeonRumor, TownRumor>;

class RumorFactory {
public:
    static Rumor create_rumor(std::span<const std::string> tokens)
    {
        const auto &category = tokens[0];
        if (category == "ARTIFACT") {
            return ArtifactRumor(tokens);
        }
        if (category == "MONSTER") {
            return MonsterRumor(tokens);
        }
        if (category == "DUNGEON") {
            return DungeonRumor(tokens);
        }
        if (category == "TOWN") {
            return TownRumor(tokens);
        }

        THROW_EXCEPTION(std::runtime_error, "Unknown token exists in rumor.txt");
    }
};

class ProcessRumor {
public:
    ProcessRumor(PlayerType *player_ptr, std::span<const std::string> tokens)
        : player_ptr(player_ptr)
        , tokens(tokens)
    {
    }

    void operator()(const ArtifactRumor &artifact_rumor)
    {
        ItemEntity item(artifact_rumor.bi_id);
        item.fa_id = artifact_rumor.fa_id;
        item.set_identification_flag(IdentificationFlag::STORE);
        const auto artifact_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
        this->print_rumor(artifact_name);
    }

    void operator()(const MonsterRumor &monster_rumor)
    {
        auto &monraces = MonraceList::get_instance();
        auto &monrace = monraces.get_monrace(monster_rumor.monrace_id);
        this->print_rumor(monrace.name);
        auto &monrace_records = MonraceRecords::get_instance();
        if (!monrace_records.has_been_seen(monster_rumor.monrace_id)) {
            monrace_records.increment_seen_count(monster_rumor.monrace_id);
        }
    }

    void operator()(const DungeonRumor &dungeon_rumor)
    {
        const auto &dungeons = DungeonList::get_instance();
        const auto &dungeon = dungeons.get_dungeon(dungeon_rumor.dungeon_id);
        const auto &dungeon_name = dungeon.name;
        this->print_rumor(dungeon_name);

        auto &dungeon_record = DungeonRecords::get_instance().get_record(dungeon_rumor.dungeon_id);
        if (!dungeon_record.has_entered()) {
            dungeon_record.set_max_level(dungeon.mindepth);
            msg_erase();
            msg_print(_("{}に帰還できるようになった。", "You can recall to {}."), dungeon_name);
        }
    }

    void operator()(const TownRumor &town_rumor)
    {
        const auto &town_name = TownList::get_instance().get_town(town_rumor.t_idx).get_name();
        this->print_rumor(town_name);

        const auto town_id = i2enum<TownId>(town_rumor.t_idx - 1);
        auto &town_records = TownRecords::get_instance();
        if ((town_rumor.t_idx != SECRET_TOWN) && !town_records.has_visited(town_id)) {
            town_records.set_visited(town_id);
            msg_erase();
            msg_print(_("{}に行ったことがある気がする。", "You feel you have been to {}."), town_name);
        }
    }

private:
    void print_rumor(std::string_view name)
    {
        const auto rumor_msg = str_replace(tokens[2], "{Name}", name);
        msg_print(rumor_msg);
    }

    PlayerType *player_ptr;
    std::span<const std::string> tokens;
};

void display_rumor(PlayerType *player_ptr, bool ex)
{
    const auto section = (ex && (randint0(3) == 0)) ? 1 : 0;
#ifdef JP
    constexpr auto max_try_count = 10;
    const auto rumor = get_random_line_ja_only("rumors_j.txt", section, max_try_count).value_or("嘘の噂もある。");
#else
    const auto rumor = get_random_line("rumors.txt", section).value_or("Some rumors are wrong.");
#endif

    if (!rumor.starts_with("R:")) {
        msg_print(rumor);
        return;
    }

    const auto tokens = get_rumor_tokens(rumor);
    if (!tokens) {
        return;
    }

    std::visit(ProcessRumor(player_ptr, *tokens), RumorFactory::create_rumor(*tokens));
}

void display_random_rumor(tl::optional<RumorRarity> rarity)
{
    const auto &rumor_list = RumorList::get_instance();
    const auto &rumor = rarity ? rumor_list.select_rumor(*rarity) : rumor_list.select_random_rumor();
    switch (rumor.get_type()) {
    case RumorType::GOSSIP:
        msg_erase();
        msg_print(rumor.get_description());
        return;
    case RumorType::TOWN:
        display_town_rumor(rumor);
        return;
    case RumorType::SHALLOW_DUNGEON:
        [[fallthrough]];
    case RumorType::DEEP_DUNGEON:
        display_dungeon_rumor(rumor);
        return;
    case RumorType::NORMAL_MONSTER:
        [[fallthrough]];
    case RumorType::UNIQUE_MONSTER:
        display_monster_rumor(rumor);
        return;
    case RumorType::SHALLOW_ARTIFACT:
        [[fallthrough]];
    case RumorType::DEEP_ARTIFACT:
        display_artifact_rumor(rumor);
        return;
    default:
        THROW_EXCEPTION(std::runtime_error, "Unknown rumor type exists in rumor list.");
    }
}
