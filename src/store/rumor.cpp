#include "store/rumor.h"
#include "artifact/fixed-art-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "object-enchant/special-object-flags.h"
#include "system/angband-exceptions.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
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

/*!
 * @brief 固定アーティファクト、モンスター、町 をランダムに1つ選ぶ
 * @param zz 検索文字列
 * @param max_idx briefに挙げた各リストにおける最大数
 * @details rumor.txt (rumor_j.txt) の定義により、常にランダム ("*")。但し拡張性のため固定値の場合も残す.
 */
static short get_rumor_num(std::string_view zz, short max_idx)
{
    if (zz == "*") {
        return randnum1<short>(max_idx);
    }

    return static_cast<short>(std::atoi(zz.data()));
}

/*!
 * @brief 噂の、町やモンスターを表すトークンを得る
 * @param rumor rumor.txt (rumor_j.txt)の1行
 * @return トークン群の配列を返す。フィールドの数が合わない場合はtl::nulloptを返す。
 * @todo tmp_tokensを使わず単なるsplitにすればもっと簡略化できそう
 */
static tl::optional<std::vector<std::string>> get_rumor_tokens(std::string rumor)
{
    constexpr auto num_tokens = 3;
    char *tmp_tokens[num_tokens];
    if (tokenize(rumor.data() + 2, num_tokens, tmp_tokens, TOKENIZE_CHECKQUOTE) != num_tokens) {
        msg_print(_("この情報は間違っている。", "This information is wrong."));
        return tl::nullopt;
    }

    std::vector<std::string> tokens(std::begin(tmp_tokens), std::end(tmp_tokens));
    return tokens;
}

class ArtifactRumor {
public:
    ArtifactRumor(std::span<const std::string> tokens)
    {
        const auto &artifact_id = tokens[1];
        const auto &artifacts = ArtifactList::get_instance();
        const auto max_idx = enum2i(artifacts.rbegin()->first);
        this->fa_id = i2enum<FixedArtifactId>(get_rumor_num(artifact_id, max_idx));
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
        const auto monraces_size = static_cast<short>(monraces.size() - 1);
        this->monrace_id = i2enum<MonraceId>(get_rumor_num(monster_name, monraces_size));
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
            this->t_idx = get_rumor_num(town_name, VALID_TOWNS);
            if (!towns_info[this->t_idx].name.empty()) {
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
        item.ident = IDENT_STORE;
        const auto artifact_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
        this->print_rumor(artifact_name);
    }

    void operator()(const MonsterRumor &monster_rumor)
    {
        auto &monraces = MonraceList::get_instance();
        auto &monrace = monraces.get_monrace(monster_rumor.monrace_id);
        this->print_rumor(monrace.name);

        if (monrace.r_sights == 0) {
            monrace.r_sights = 1;
        }
    }

    void operator()(const DungeonRumor &dungeon_rumor)
    {
        const auto &dungeons = DungeonList::get_instance();
        const auto &dungeon = dungeons.get_dungeon(dungeon_rumor.dungeon_id);
        this->print_rumor(dungeon.name);

        auto &dungeon_record = DungeonRecords::get_instance().get_record(dungeon_rumor.dungeon_id);
        if (!dungeon_record.has_entered()) {
            dungeon_record.set_max_level(dungeon.mindepth);
            msg_erase();
            msg_print(_("{}に帰還できるようになった。", "You can recall to {}."), dungeon.name);
        }
    }

    void operator()(const TownRumor &town_rumor)
    {
        const auto &town_name = towns_info[town_rumor.t_idx].name;
        this->print_rumor(town_name);

        const uint32_t visit = (1U << (town_rumor.t_idx - 1));
        if ((town_rumor.t_idx != SECRET_TOWN) && !(player_ptr->visit & visit)) {
            player_ptr->visit |= visit;
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
